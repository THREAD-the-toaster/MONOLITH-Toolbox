#pragma once
#include <shlobj.h>
#include <wincodec.h>
#include <array>
#include <cmath>

// Keep native icon handles across render-target recreation. Direct2D bitmaps belong to rt.
namespace NativeIcons {
enum { Code, Blender, Git, Unreal, Godot, Python, Cad, Cuda, CMake, Embedded, Custom, Brand, NavHome, NavAvailable, NavInstalled, NavProject, NavUpdates, NavDownloads, NavSettings, UiSearch, UiPlay, UiChevron, UiGrid, UiMinimize, UiMaximize, UiRestore, UiClose, Count };
struct Entry {HICON icon=nullptr;IWICBitmapFrameDecode* image=nullptr;ID2D1Bitmap* bitmap=nullptr;bool attempted=false;UINT rasterSize=0;};
static std::array<Entry,Count> entries;
static IWICImagingFactory* imaging=nullptr;
static std::wstring environment(const wchar_t* name){
    DWORD count=GetEnvironmentVariableW(name,nullptr,0);if(!count)return {};
    std::wstring value(count,L'\0');GetEnvironmentVariableW(name,value.data(),count);value.resize(count-1);return value;
}
static std::wstring registryString(HKEY root,const std::wstring& key,const wchar_t* value,REGSAM view=0){
    HKEY handle=nullptr;if(RegOpenKeyExW(root,key.c_str(),0,KEY_READ|view,&handle)!=ERROR_SUCCESS)return {};
    DWORD bytes=0,type=0;std::wstring result;
    if(RegQueryValueExW(handle,value,nullptr,&type,nullptr,&bytes)==ERROR_SUCCESS&&(type==REG_SZ||type==REG_EXPAND_SZ)&&bytes>=sizeof(wchar_t)){
        result.resize(bytes/sizeof(wchar_t));
        if(RegQueryValueExW(handle,value,nullptr,&type,reinterpret_cast<BYTE*>(result.data()),&bytes)!=ERROR_SUCCESS)result.clear();
        while(!result.empty()&&result.back()==L'\0')result.pop_back();
    }
    RegCloseKey(handle);return result;
}
static HICON fromResource(std::wstring resource){
    if(resource.empty())return nullptr;
    DWORD count=ExpandEnvironmentStringsW(resource.c_str(),nullptr,0);
    if(!count)return nullptr;
    std::wstring expanded(count,L'\0');ExpandEnvironmentStringsW(resource.c_str(),expanded.data(),count);expanded.resize(count-1);
    int index=0;size_t comma=expanded.rfind(L',');
    if(comma!=std::wstring::npos){wchar_t* end=nullptr;long parsed=wcstol(expanded.c_str()+comma+1,&end,10);if(end!=expanded.c_str()+comma+1&&*end==0){index=int(parsed);expanded.resize(comma);}}
    size_t first=expanded.find_first_not_of(L" \t\"");size_t last=expanded.find_last_not_of(L" \t\"");
    if(first==std::wstring::npos)return nullptr;
    expanded=expanded.substr(first,last-first+1);
    std::error_code error;if(!fs::is_regular_file(fs::path(expanded),error))return nullptr;
    HICON icon=nullptr;SHDefExtractIconW(expanded.c_str(),index,0,&icon,nullptr,128);return icon;
}
static HICON stock(SHSTOCKICONID id){SHSTOCKICONINFO info{};info.cbSize=sizeof(info);return SUCCEEDED(SHGetStockIconInfo(id,SHGSI_ICON|SHGSI_LARGEICON,&info))?info.hIcon:nullptr;}
static std::wstring lowercase(std::wstring text){std::transform(text.begin(),text.end(),text.begin(),towlower);return text;}
static HICON application(std::initializer_list<const wchar_t*> exeNames,std::initializer_list<const wchar_t*> displayNames,std::initializer_list<fs::path> paths){
    for(auto exe:exeNames){
        for(HKEY root:{HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE})for(REGSAM view:{REGSAM(KEY_WOW64_64KEY),REGSAM(KEY_WOW64_32KEY)}){
            auto path=registryString(root,L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\"+std::wstring(exe),nullptr,view);
            if(auto icon=fromResource(path))return icon;
        }
        wchar_t found[32768]{};DWORD size=SearchPathW(nullptr,exe,nullptr,32768,found,nullptr);
        if(size&&size<32768&&lowercase(found).find(L"microsoft\\windowsapps\\")==std::wstring::npos){if(auto icon=fromResource(found))return icon;}
    }
    for(const auto& path:paths){
        if(auto icon=fromResource(path.wstring()))return icon;
        // Versioned installations (Blender, Python, Unreal, CUDA): bounded, one-level scan.
        std::error_code error;if(!fs::is_directory(path,error))continue;
        std::vector<fs::path> directories;
        for(fs::directory_iterator it(path,fs::directory_options::skip_permission_denied,error),end;it!=end&&!error;it.increment(error)){
            if(it->is_directory(error))directories.push_back(it->path());
            if(directories.size()>=100)break;
        }
        std::sort(directories.rbegin(),directories.rend());
        for(const auto& directory:directories)for(auto exe:exeNames){if(auto icon=fromResource((directory/exe).wstring()))return icon;}
    }
    // Respect custom install locations advertised by each application's uninstaller.
    const std::wstring base=L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    for(HKEY root:{HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE})for(REGSAM view:{REGSAM(KEY_WOW64_64KEY),REGSAM(KEY_WOW64_32KEY)}){
        HKEY handle=nullptr;if(RegOpenKeyExW(root,base.c_str(),0,KEY_READ|view,&handle)!=ERROR_SUCCESS)continue;
        HICON found=nullptr;
        for(DWORD n=0;n<4096&&!found;n++){
            wchar_t name[512]{};DWORD length=512;LONG result=RegEnumKeyExW(handle,n,name,&length,nullptr,nullptr,nullptr,nullptr);
            if(result==ERROR_NO_MORE_ITEMS)break;
            if(result!=ERROR_SUCCESS)continue;
            auto key=base+L"\\"+name;auto display=lowercase(registryString(root,key,L"DisplayName",view));
            bool matches=false;for(auto expected:displayNames)if(display.find(lowercase(expected))!=std::wstring::npos){matches=true;break;}
            if(!matches)continue;
            found=fromResource(registryString(root,key,L"DisplayIcon",view));
            if(!found){auto location=registryString(root,key,L"InstallLocation",view);if(!location.empty())for(auto exe:exeNames){found=fromResource((fs::path(location)/exe).wstring());if(found)break;}}
        }
        RegCloseKey(handle);if(found)return found;
    }
    return nullptr;
}
static void initialize(){
    CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&imaging));
    fs::path programs=environment(L"ProgramFiles"),local=environment(L"LOCALAPPDATA"),programs86=environment(L"ProgramFiles(x86)");
    entries[Code].icon=application({L"Code.exe"},{L"Microsoft Visual Studio Code"},{local/L"Programs/Microsoft VS Code/Code.exe",programs/L"Microsoft VS Code/Code.exe"});
    entries[Blender].icon=application({L"blender.exe"},{L"Blender"},{programs/L"Blender Foundation"});
    entries[Git].icon=application({L"git-gui.exe",L"git.exe"},{L"Git version",L"Git_is1"},{programs/L"Git/cmd/git-gui.exe",programs/L"Git/cmd/git.exe",local/L"Programs/Git/cmd/git.exe"});
    entries[Unreal].icon=application({L"Engine/Binaries/Win64/UnrealEditor.exe",L"UnrealEditor.exe"},{L"Unreal Engine"},{programs/L"Epic Games"});
    entries[Godot].icon=application({L"Godot.exe",L"godot.exe"},{L"Godot"},{local/L"Programs/Godot",programs/L"Godot"});
    entries[Python].icon=application({L"python.exe",L"pythonw.exe"},{L"Python 3"},{local/L"Programs/Python",programs,programs86});
    entries[Cad].icon=application({L"MONOLITH-CAD.exe"},{L"MONOLITH CAD"},{programs/L"MONOLITH/MONOLITH-CAD.exe"});
    entries[Cuda].icon=application({L"bin/nvvp.exe",L"bin/nsight.exe",L"nvcc.exe"},{L"CUDA Toolkit"},{programs/L"NVIDIA GPU Computing Toolkit/CUDA"});
    entries[CMake].icon=application({L"cmake-gui.exe",L"bin/cmake-gui.exe"},{L"CMake"},{programs/L"CMake",local/L"Programs/CLion/bin/cmake/win/x64"});
    entries[Embedded].icon=stock(SIID_SOFTWARE);entries[Custom].icon=stock(SIID_FOLDER);
    wchar_t module[32768]{};GetModuleFileNameW(nullptr,module,32768);fs::path executable(module);
    for(auto path:{executable.parent_path()/L"assets/monolith.ico",executable.parent_path().parent_path()/L"assets/monolith.ico"})if(!entries[Brand].icon)entries[Brand].icon=fromResource(path.wstring());
    // Load the supplied PNG as-is. WIC preserves its original pixels and alpha channel.
    for(auto path:{executable.parent_path()/L"assets/monolith.png",executable.parent_path().parent_path()/L"assets/monolith.png"}){
        IWICBitmapDecoder* decoder=nullptr;
        if(imaging&&SUCCEEDED(imaging->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder))){
            decoder->GetFrame(0,&entries[Brand].image);decoder->Release();
            if(entries[Brand].image)break;
        }
    }
    const SHSTOCKICONID navigation[]={SIID_FOLDEROPEN,SIID_SOFTWARE,SIID_DESKTOPPC,SIID_DOCASSOC,SIID_NETWORKCONNECT,SIID_DRIVEFIXED,SIID_SETTINGS};
    for(int i=0;i<7;i++)entries[NavHome+i].icon=stock(navigation[i]);
    if(entries[NavSettings].icon)DestroyIcon(entries[NavSettings].icon);
    entries[NavSettings].icon=fromResource((fs::path(environment(L"SystemRoot"))/L"System32/control.exe").wstring());
    const wchar_t* uiNames[]={L"home",L"package",L"monitor",L"plus",L"refresh",L"download",L"settings",L"search",L"play",L"chevron",L"grid",L"minimize",L"maximize",L"restore",L"close"};
    for(int i=0;i<15;i++){
        auto& entry=entries[NavHome+i];
        for(auto root:{executable.parent_path()/L"assets/ui",executable.parent_path().parent_path()/L"assets/ui"}){
            IWICBitmapDecoder* decoder=nullptr;
            if(imaging&&SUCCEEDED(imaging->CreateDecoderFromFilename((root/(std::wstring(uiNames[i])+L".png")).c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder))){
                decoder->GetFrame(0,&entry.image);decoder->Release();if(entry.image)break;
            }
        }
    }
    for(auto& entry:entries)if(!entry.icon)entry.icon=stock(SIID_APPLICATION);
}
static void resetBitmaps(){for(auto& entry:entries){release(entry.bitmap);entry.attempted=false;entry.rasterSize=0;}}
static void shutdown(){resetBitmaps();for(auto& entry:entries){if(entry.icon)DestroyIcon(entry.icon);entry.icon=nullptr;release(entry.image);}release(imaging);}
static void draw(int id,float x,float y,float size,float opacity=1.f){
    if(id<0||id>=Count||!rt)return;
    auto& entry=entries[id];
    // Rasterize the original logo at the monitor's physical pixel size, not at
    // a fixed thumbnail size. The surrounding layout remains in logical DIPs.
    const float monitorScale=std::max(.01f,dpiScale);
    UINT requestedPixels=UINT(std::max(1L,std::lround(size*monitorScale)));
    if(entry.image&&entry.rasterSize!=requestedPixels){
        release(entry.bitmap);entry.attempted=false;entry.rasterSize=requestedPixels;
    }
    if(!entry.attempted){
        entry.attempted=true;
        IWICBitmap* source=nullptr;IWICFormatConverter* converted=nullptr;IWICBitmapScaler* scaler=nullptr;
        IWICBitmapSource* input=entry.image;
        if(!input&&imaging&&entry.icon&&SUCCEEDED(imaging->CreateBitmapFromHICON(entry.icon,&source)))input=source;
        if(input&&SUCCEEDED(imaging->CreateFormatConverter(&converted))&&
           SUCCEEDED(converted->Initialize(input,GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom))){
            IWICBitmapSource* raster=converted;
            if(entry.image){
                UINT width=0,height=0;input->GetSize(&width,&height);
                if(width&&height&&SUCCEEDED(imaging->CreateBitmapScaler(&scaler))){
                    double ratio=double(requestedPixels)/std::max(width,height);
                    UINT targetWidth=UINT(std::max(1L,std::lround(width*ratio)));
                    UINT targetHeight=UINT(std::max(1L,std::lround(height*ratio)));
                    if(SUCCEEDED(scaler->Initialize(converted,targetWidth,targetHeight,WICBitmapInterpolationModeFant)))raster=scaler;
                }
            }
            // Ignore PNG print-resolution metadata: Windows monitor DPI is the
            // only display scale. Premultiplied alpha avoids dark edge fringes.
            auto properties=D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED),96.f,96.f);
            rt->CreateBitmapFromWicBitmap(raster,&properties,&entry.bitmap);
        }
        release(scaler);release(converted);release(source);
    }
    if(entry.bitmap){
        auto pixels=entry.bitmap->GetPixelSize();
        if(!pixels.width||!pixels.height)return;
        float fit=size/float(std::max(pixels.width,pixels.height));
        float width=pixels.width*fit,height=pixels.height*fit;
        float left=x+(size-width)*.5f,top=y+(size-height)*.5f;
        if(entry.image){
            // One image pixel maps to one physical pixel, including at 125/150%.
            width=pixels.width/monitorScale;height=pixels.height/monitorScale;
            left=std::round((x+(size-width)*.5f)*monitorScale)/monitorScale;
            top=std::round((y+(size-height)*.5f)*monitorScale)/monitorScale;
        }
        rt->DrawBitmap(entry.bitmap,rect(left,top,width,height),opacity,D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }
}
static int projectIcon(const std::wstring& name){
    if(name==L"C++ / CMake")return CMake;
    if(name==L"Python")return Python;
    if(name==L"Godot")return Godot;
    if(name==L"Blender")return Blender;
    if(name==L"Embedded")return Embedded;
    return Custom;
}
}






