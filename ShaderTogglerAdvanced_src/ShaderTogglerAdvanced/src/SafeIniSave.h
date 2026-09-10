// Copyright (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// Proprietary modifications; see PROPRIETARY_LICENSE.txt in the source package.
#pragma once
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace ShaderToggler::ini_save
{
    struct Result
    {
        bool saved=false;
        const char* step="create temporary file";
        uint32_t error=0;
    };

    template<class Files> struct TemporaryFile
    {
        Files& files;
        std::filesystem::path path;
        bool owned=false;
        explicit TemporaryFile(Files& value):files(value) {}
        ~TemporaryFile() { if(owned) files.remove(path); }
        TemporaryFile(const TemporaryFile&)=delete;
        TemporaryFile& operator=(const TemporaryFile&)=delete;
    };

    template<class Files,class Writer>
    Result save(const std::filesystem::path& destination,Files& files,Writer&& write)
    {
        Result result;
        TemporaryFile<Files> pending(files),backup(files);
        try
        {
            if(!files.stage(destination,pending.path,result.error)) return result;
            pending.owned=true;
            result.step="write configuration";
            if(!write(pending.path)) return result;
            result.step="flush configuration";
            if(!files.flush(pending.path,result.error)) return result;
            result.step="inspect existing configuration";
            bool exists=false;
            if(!files.exists(destination,exists,result.error)) return result;
            if(exists)
            {
                auto backupName=destination;backupName+=".bak";
                result.step="create backup temporary file";
                if(!files.stage(backupName,backup.path,result.error)) return result;
                backup.owned=true;
                result.step="copy previous configuration";
                if(!files.copy(destination,backup.path,result.error)) return result;
                result.step="flush backup";
                if(!files.flush(backup.path,result.error)) return result;
                result.step="replace backup";
                if(!files.replace(backup.path,backupName,result.error)) return result;
                backup.owned=false;
            }
            result.step="replace configuration";
            if(!files.replace(pending.path,destination,result.error)) return result;
            pending.owned=false;
            result.saved=true;result.step="complete";result.error=0;
        }
        catch(...)
        {
            result.saved=false;
        }
        return result;
    }

#ifdef _WIN32
    struct WindowsFiles
    {
        bool stage(const std::filesystem::path& destination,std::filesystem::path& out,uint32_t& error)
        {
            static std::atomic_uint64_t serial{0};
            for(unsigned attempt=0;attempt<32;++attempt)
            {
                auto path=destination;
                path+=L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(++serial);
                const auto name=path.wstring();
                HANDLE file=CreateFileW(name.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
                if(file!=INVALID_HANDLE_VALUE)
                {
                    if(!CloseHandle(file)) { error=GetLastError();DeleteFileW(name.c_str());return false; }
                    out=std::move(path);error=0;return true;
                }
                error=GetLastError();
                if(error!=ERROR_FILE_EXISTS && error!=ERROR_ALREADY_EXISTS) return false;
            }
            return false;
        }
        bool exists(const std::filesystem::path& path,bool& present,uint32_t& error)
        {
            const auto name=path.wstring();
            const DWORD attributes=GetFileAttributesW(name.c_str());
            if(attributes!=INVALID_FILE_ATTRIBUTES)
            {
                if(attributes&FILE_ATTRIBUTE_DIRECTORY) {error=ERROR_DIRECTORY;return false;}
                if(attributes&FILE_ATTRIBUTE_READONLY) {error=ERROR_ACCESS_DENIED;return false;}
                present=true;error=0;return true;
            }
            error=GetLastError();present=false;
            if(error==ERROR_FILE_NOT_FOUND) {error=0;return true;}
            return false;
        }
        bool copy(const std::filesystem::path& from,const std::filesystem::path& to,uint32_t& error)
        {
            const auto source=from.wstring(),target=to.wstring();
            if(CopyFileW(source.c_str(),target.c_str(),FALSE)) {error=0;return true;}
            error=GetLastError();return false;
        }
        bool flush(const std::filesystem::path& path,uint32_t& error)
        {
            const auto name=path.wstring();
            HANDLE file=CreateFileW(name.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(file==INVALID_HANDLE_VALUE) {error=GetLastError();return false;}
            const bool flushed=FlushFileBuffers(file)!=FALSE;
            if(!flushed) error=GetLastError();
            const bool closed=CloseHandle(file)!=FALSE;
            if(!closed && flushed) error=GetLastError();
            if(flushed && closed) {error=0;return true;}
            return false;
        }
        bool replace(const std::filesystem::path& from,const std::filesystem::path& to,uint32_t& error)
        {
            const auto source=from.wstring(),target=to.wstring();
            if(MoveFileExW(source.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {error=0;return true;}
            error=GetLastError();return false;
        }
        void remove(const std::filesystem::path& path) noexcept
        {
            DeleteFileW(path.c_str());
        }
    };
#endif
}
