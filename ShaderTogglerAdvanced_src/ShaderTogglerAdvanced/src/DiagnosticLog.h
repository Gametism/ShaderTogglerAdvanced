// Copyright (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// See PROPRIETARY_LICENSE.txt. Diagnostics do not intercept exceptions or rendering.
#pragma once
#include <Windows.h>
#include <atomic>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwchar>

namespace ShaderToggler::diagnostics
{
	inline constexpr wchar_t FileName[] = L"ShaderTogglerAdvanced.log";
	inline constexpr unsigned long MaxBytes = 2 * 1024 * 1024;
	inline SRWLOCK writerLock = SRWLOCK_INIT;
	inline HANDLE file = INVALID_HANDLE_VALUE;
	inline wchar_t logPath[32768]{};
	inline std::atomic<bool> recording{false};
	inline std::atomic<bool> capped{false};
	inline std::atomic<DWORD> error{0};
	inline std::atomic<unsigned long> dropped{0};
	inline unsigned long bytesWritten = 0;
	inline unsigned linesInWindow = 0;
	inline ULONGLONG startedAt = 0, windowStart = 0;

	struct PreserveError
	{
		DWORD windowsError = GetLastError();
		int crtError = errno;
		~PreserveError() { SetLastError(windowsError); errno = crtError; }
	};

	struct TryWriter
	{
		bool acquired = TryAcquireSRWLockExclusive(&writerLock) != FALSE;
		~TryWriter() { if (acquired) ReleaseSRWLockExclusive(&writerLock); }
	};

	// Called only while holding writerLock. Never retry a failed/partial write.
	inline bool writeBytes(const char* data, DWORD length) noexcept
	{
		DWORD written = 0;
		const BOOL ok = WriteFile(file, data, length, &written, nullptr);
		bytesWritten += written;
		if (!ok || written != length)
		{
			error.store(ok ? ERROR_WRITE_FAULT : GetLastError(), std::memory_order_relaxed);
			recording.store(false, std::memory_order_relaxed);
			return false;
		}
		return true;
	}

	inline void Write(const char* format, ...) noexcept
	{
		PreserveError preserve;
		if (!recording.load(std::memory_order_relaxed))
			return;
		TryWriter lock;
		if (!lock.acquired)
		{
			dropped.fetch_add(1, std::memory_order_relaxed);
			return;
		}
		if (!recording.load(std::memory_order_relaxed))
			return;
		const ULONGLONG now = GetTickCount64();
		if (now - windowStart >= 1000)
		{
			windowStart = now;
			linesInWindow = 0;
		}
		const bool important = std::strncmp(format, "[ERROR]", 7) == 0 ||
			std::strncmp(format, "[INIT]", 6) == 0 || std::strncmp(format, "[READY]", 7) == 0 ||
			std::strncmp(format, "[STOP]", 6) == 0 || std::strncmp(format, "[SHUTDOWN]", 10) == 0;
		if (!important && linesInWindow >= 128)
		{
			dropped.fetch_add(1, std::memory_order_relaxed);
			return;
		}

		char message[1024]{};
		va_list args;
		va_start(args, format);
		const int formatted = std::vsnprintf(message, sizeof(message), format, args);
		va_end(args);
		if (formatted < 0)
			std::strcpy(message, "[LOG] A diagnostic message could not be formatted.");
		// Keep user-supplied group names on a single physical log line.
		for (char* p = message; *p; ++p)
			if (static_cast<unsigned char>(*p) < 32) *p = ' ';

		char line[1280]{};
		const unsigned long skipped = dropped.exchange(0, std::memory_order_relaxed);
		const int length = std::snprintf(line, sizeof(line), "[%llu ms] [thread %lu] [skipped %lu] %s\r\n",
			static_cast<unsigned long long>(now - startedAt), static_cast<unsigned long>(GetCurrentThreadId()), skipped, message);
		if (length <= 0 || length >= static_cast<int>(sizeof(line)))
			return;
		constexpr char limitNotice[] = "[LOG] 2 MiB limit reached; further logging disabled for this run.\r\n";
		if (bytesWritten + static_cast<unsigned long>(length) > MaxBytes - sizeof(limitNotice))
		{
			if (writeBytes(limitNotice, static_cast<DWORD>(sizeof(limitNotice) - 1)))
				capped.store(true, std::memory_order_relaxed);
			recording.store(false, std::memory_order_relaxed);
			return;
		}
		if (writeBytes(line, static_cast<DWORD>(length)))
			++linesInWindow;
	}

	inline void WritePath(const char* label, const wchar_t* value) noexcept
	{
		PreserveError preserve;
		char utf8[900]{};
		if (WideCharToMultiByte(CP_UTF8, 0, value, -1, utf8, sizeof(utf8), nullptr, nullptr) == 0)
			Write("[PATH] %s: path omitted (conversion failed or path exceeds diagnostic line length).", label);
		else
			Write("[PATH] %s: %s", label, utf8);
	}

	// Initialize once before registering callbacks. No background thread or
	// crash handler is installed. Refuse to truncate a log held by another writer.
	inline bool Start() noexcept
	{
		PreserveError preserve;
		if (file != INVALID_HANDLE_VALUE)
			return recording.load(std::memory_order_relaxed);
		const DWORD capacity = static_cast<DWORD>(sizeof(logPath) / sizeof(logPath[0]));
		const DWORD length = GetModuleFileNameW(nullptr, logPath, capacity);
		if (length == 0 || length >= capacity)
		{
			error.store(length == 0 ? GetLastError() : ERROR_INSUFFICIENT_BUFFER);
			return false;
		}
		wchar_t* leaf = std::wcsrchr(logPath, L'\\');
		if (!leaf) leaf = std::wcsrchr(logPath, L'/');
		leaf = leaf ? leaf + 1 : logPath;
		if (static_cast<size_t>(leaf - logPath) + sizeof(FileName) / sizeof(FileName[0]) > capacity)
		{
			error.store(ERROR_INSUFFICIENT_BUFFER);
			return false;
		}
		std::wcscpy(leaf, FileName);
		file = CreateFileW(logPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE,
			nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			error.store(GetLastError());
			return false;
		}
		bytesWritten = linesInWindow = 0;
		startedAt = windowStart = GetTickCount64();
		dropped.store(0);
		error.store(0);
		capped.store(false);
		recording.store(true);
		SYSTEMTIME utc{};
		GetSystemTime(&utc);
		Write("[START] Shader Toggler Advanced | version 1.5.0.0 | %u-bit | PID %lu | UTC %04u-%02u-%02u %02u:%02u:%02u",
			static_cast<unsigned>(sizeof(void*) * 8), static_cast<unsigned long>(GetCurrentProcessId()),
			utc.wYear, utc.wMonth, utc.wDay, utc.wHour, utc.wMinute, utc.wSecond);
		Write("[LOG] Previous run overwritten. Limit 2 MiB. No per-draw/dispatch logging; no crash interception. Skipped counts indicate rate limiting or concurrent logging.");
		return recording.load(std::memory_order_relaxed);
	}

	inline void Stop(bool processExit) noexcept
	{
		PreserveError preserve;
		Write("[STOP] Add-on detach; process_exit=%u. This entry is not a crash diagnosis.", processExit ? 1u : 0u);
		TryWriter lock;
		if (!lock.acquired) return; // Never wait for another thread inside DllMain.
		recording.store(false, std::memory_order_relaxed);
		if (file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
			file = INVALID_HANDLE_VALUE;
		}
	}
}
