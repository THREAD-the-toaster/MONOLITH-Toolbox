#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <stdexcept>
#include <windowsx.h>
// Resolve newer Windows DPI APIs at runtime for MinGW header compatibility.
static void enableDpiAwareness() {
    HMODULE user = GetModuleHandleW(L"user32.dll");
    if (!user) return;
    using SetDpiContextFn = BOOL (WINAPI*)(HANDLE);
    auto fn = reinterpret_cast<SetDpiContextFn>(GetProcAddress(user, "SetProcessDpiAwarenessContext"));
    // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 == (HANDLE)-4
    if (fn) fn(reinterpret_cast<HANDLE>(static_cast<intptr_t>(-4)));
}
static float windowDpiScale(HWND hwnd) {
    HMODULE user = GetModuleHandleW(L"user32.dll");
    if (!user) return 1.0f;
    using GetDpiFn = UINT (WINAPI*)(HWND);
    auto fn = reinterpret_cast<GetDpiFn>(GetProcAddress(user, "GetDpiForWindow"));
    return fn ? static_cast<float>(fn(hwnd)) / 96.0f : 1.0f;
}
namespace fs = std::filesystem;
template<class T> void release(T*& p){if(p){p->Release();p=nullptr;}}
struct Tile {std::wstring name,subtitle; D2D1_COLOR_F accent; std::vector<std::wstring> uses;};
static std::vector<Tile> tiles={
 {L"C++ / CMake",L"Native application",D2D1::ColorF(0.25f,0.64f,1.f),{L"Console",L"Desktop application",L"Static library"}},
 {L"Python",L"Scripting workspace",D2D1::ColorF(1.f,0.78f,0.23f),{L"Basic script",L"Python package"}},
 {L"Godot",L"Game development",D2D1::ColorF(0.25f,0.70f,0.98f),{L"2D game",L"3D game"}},
 {L"Blender",L"3D creation",D2D1::ColorF(1.f,0.53f,0.18f),{L"3D workspace",L"Animation workspace"}},
 {L"Embedded",L"Firmware workspace",D2D1::ColorF(0.42f,0.88f,0.66f),{L"CMake firmware",L"Empty embedded"}},
 {L"Custom",L"Empty local project",D2D1::ColorF(0.77f,0.52f,1.f),{L"Empty workspace"}}
};
static HWND windowHandle=nullptr; static ID2D1Factory* factory=nullptr; static ID2D1HwndRenderTarget* rt=nullptr; static IDWriteFactory* writeFactory=nullptr; static IDWriteTextFormat* body=nullptr; static IDWriteTextFormat* titleFormat=nullptr;
static int page=0, selected=-1, drag=-1; static bool menuOpen=false; static float dpiScale=1.f; static std::wstring status=L"Ready  |  Network idle";
static D2D1_COLOR_F rgb(unsigned hex,float a=1.f){return D2D1::ColorF(((hex>>16)&255)/255.f,((hex>>8)&255)/255.f,(hex&255)/255.f,a);}
static D2D1_RECT_F rect(float x,float y,float w,float h){return D2D1::RectF(x,y,x+w,y+h);}
static void rounded(float x,float y,float w,float h,float r,D2D1_COLOR_F color){ID2D1SolidColorBrush* b=nullptr;rt->CreateSolidColorBrush(color,&b);if(b){rt->FillRoundedRectangle(D2D1::RoundedRect(rect(x,y,w,h),r,r),b);b->Release();}}
static void stroke(float x,float y,float w,float h,float r,D2D1_COLOR_F color,float thickness=1){ID2D1SolidColorBrush* b=nullptr;rt->CreateSolidColorBrush(color,&b);if(b){rt->DrawRoundedRectangle(D2D1::RoundedRect(rect(x,y,w,h),r,r),b,thickness);b->Release();}}
#include "native_icons.h"
static void resetTarget(){NativeIcons::resetBitmaps();release(rt);}
static void initTarget(){if(rt||!factory)return;RECT c{};GetClientRect(windowHandle,&c);factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(), 96.f, 96.f),D2D1::HwndRenderTargetProperties(windowHandle,D2D1::SizeU(std::max(1L,c.right),std::max(1L,c.bottom))),&rt);}
static float cw(){RECT r{};GetClientRect(windowHandle,&r);return float(r.right)/dpiScale;} static float ch(){RECT r{};GetClientRect(windowHandle,&r);return float(r.bottom)/dpiScale;}
// Layout and pointer coordinates use logical pixels; the target stays at 96 DPI.
static int tileColumns(){return cw()-292.f >= 780.f ? 3 : 2;}
static D2D1_RECT_F tileRect(int i){
    int columns=tileColumns();
    float width=(cw()-292.f-(columns-1)*16.f)/columns;
    return rect(260.f+(i%columns)*(width+16.f),196.f+(i/columns)*168.f,width,152.f);
}static bool inside(float x,float y,D2D1_RECT_F r){return x>=r.left&&x<=r.right&&y>=r.top&&y<=r.bottom;}
#include "ui.h"
static std::wstring safe(const std::wstring& in){std::wstring out;for(wchar_t c:in){if(c<32||wcschr(L"<>:\"/\\|?*",c))return L"";out+=c;}while(!out.empty()&&(out.back()==L'.'||out.back()==L' '))out.pop_back();if(out.empty()||out==L"."||out==L"..")return L"";std::wstring upper=out;std::transform(upper.begin(),upper.end(),upper.begin(),towupper);if(upper==L"CON"||upper==L"PRN"||upper==L"AUX"||upper==L"NUL"||upper==L"COM1"||upper==L"LPT1")return L"";return out;}
static bool writeNew(const fs::path& p,const std::string& data){std::ofstream f(p,std::ios::binary|std::ios::trunc);if(!f)return false;f.write(data.data(),(std::streamsize)data.size());f.close();return bool(f);}
static HWND inputWindow=nullptr,inputName=nullptr,inputPath=nullptr;static int pendingTile=-1,pendingUse=0;
static LRESULT CALLBACK inputProc(HWND h,UINT msg,WPARAM wp,LPARAM lp){switch(msg){case WM_CREATE:{HFONT font=(HFONT)GetStockObject(DEFAULT_GUI_FONT);CreateWindowW(L"STATIC",L"Project name",WS_CHILD|WS_VISIBLE,18,17,400,20,h,nullptr,nullptr,nullptr);inputName=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"NewProject",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,18,41,420,29,h,(HMENU)1,nullptr,nullptr);CreateWindowW(L"STATIC",L"Workspace root directory",WS_CHILD|WS_VISIBLE,18,85,400,20,h,nullptr,nullptr,nullptr);wchar_t docs[MAX_PATH]{};GetEnvironmentVariableW(L"USERPROFILE",docs,MAX_PATH);std::wstring def=std::wstring(docs)+L"\\Documents\\MONOLITH Projects";inputPath=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",def.c_str(),WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,18,108,420,29,h,(HMENU)2,nullptr,nullptr);HWND ok=CreateWindowW(L"BUTTON",L"Create workspace",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,252,164,186,35,h,(HMENU)3,nullptr,nullptr);HWND cancel=CreateWindowW(L"BUTTON",L"Cancel",WS_CHILD|WS_VISIBLE,150,164,92,35,h,(HMENU)4,nullptr,nullptr);for(HWND c:{inputName,inputPath,ok,cancel})SendMessageW(c,WM_SETFONT,(WPARAM)font,TRUE);return 0;}
 case WM_COMMAND:if(LOWORD(wp)==4){DestroyWindow(h);return 0;}if(LOWORD(wp)==3){wchar_t n[260]{},p[32768]{};GetWindowTextW(inputName,n,260);GetWindowTextW(inputPath,p,32768);std::wstring sn=safe(n);if(sn.empty()){MessageBoxW(h,L"Invalid project name.",L"MONOLITH",MB_ICONWARNING);return 0;}try{fs::path root(p);if(!root.is_absolute()||root.empty())throw std::runtime_error("Invalid path");fs::create_directories(root);fs::path project=root/sn;if(fs::exists(project))throw std::runtime_error("Project folder already exists");fs::create_directory(project);try{fs::create_directories(project/L".monolith");fs::create_directories(project/L"src");fs::create_directories(project/L"assets");fs::create_directories(project/L"tests");std::string cmake="cmake_minimum_required(VERSION 3.21)\nproject(QuickProject LANGUAGES CXX)\nset(CMAKE_CXX_STANDARD 20)\nadd_executable(QuickProject src/main.cpp)\n";std::string source="#include <iostream>\nint main(){std::cout << \"Hello from MONOLITH!\\n\";}\n";if(!writeNew(project/L"README.md","# MONOLITH Workspace\n\nCreated locally by MONOLITH Toolbox.\n")||!writeNew(project/L".gitignore","build/\n.idea/\n")||!writeNew(project/L".monolith"/L"workspace.mlw","MLW1\nlocal-workspace\n")||!writeNew(project/L"CMakeLists.txt",cmake)||!writeNew(project/L"src"/L"main.cpp",source))throw std::runtime_error("Cannot write files");}catch(...){fs::remove_all(project);throw;}status=L"Workspace created: "+project.wstring();ShellExecuteW(h,L"open",project.c_str(),nullptr,nullptr,SW_SHOWNORMAL);DestroyWindow(h);}catch(const std::exception&){MessageBoxW(h,L"Unable to create workspace. Check directory permissions, path and existing folders.",L"MONOLITH",MB_ICONERROR);}return 0;}break;
 case WM_DESTROY:inputWindow=nullptr;EnableWindow(windowHandle,TRUE);SetForegroundWindow(windowHandle);InvalidateRect(windowHandle,nullptr,FALSE);return 0;}return DefWindowProcW(h,msg,wp,lp);}
static void openWizard(int i,int use){pendingTile=i;pendingUse=use;if(inputWindow)return;static bool registered=false;if(!registered){WNDCLASSW wc{};wc.lpfnWndProc=inputProc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"MONOLITHWorkspaceWizard";wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);registered=true;}inputWindow=CreateWindowExW(WS_EX_DLGMODALFRAME,L"MONOLITHWorkspaceWizard",(L"New " + tiles[i].name+L" workspace - "+tiles[i].uses[use]).c_str(),WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,240,170,470,255,windowHandle,nullptr,GetModuleHandleW(nullptr),nullptr);EnableWindow(windowHandle,FALSE);ShowWindow(inputWindow,SW_SHOW);}
static LRESULT CALLBACK wndProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){case WM_CREATE:{windowHandle=h;D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,&factory);DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),(IUnknown**)&writeFactory);if(writeFactory){writeFactory->CreateTextFormat(L"Segoe UI",nullptr,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,14.f,L"en-US",&body);writeFactory->CreateTextFormat(L"Segoe UI",nullptr,DWRITE_FONT_WEIGHT_SEMI_BOLD,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,25.f,L"en-US",&titleFormat);}NativeIcons::initialize();dpiScale=windowDpiScale(h);return 0;}
 case WM_DPICHANGED:{dpiScale=float(HIWORD(w))/96.f;resetTarget();RECT* r=(RECT*)l;SetWindowPos(h,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);InvalidateRect(h,nullptr,FALSE);return 0;}
 case WM_GETMINMAXINFO:{
    auto info=reinterpret_cast<MINMAXINFO*>(l);
    float scale=windowDpiScale(h);
    RECT minimum{0,0,LONG(1000*scale),LONG(760*scale)};
    AdjustWindowRectEx(&minimum,WS_OVERLAPPEDWINDOW,FALSE,0);
    info->ptMinTrackSize={minimum.right-minimum.left,minimum.bottom-minimum.top};
    return 0;
 }
 case WM_SIZE:menuOpen=false;hoverControl=pressedControl=-1;catalogueScroll=0;if(rt)rt->Resize(D2D1::SizeU(LOWORD(l),HIWORD(l)));InvalidateRect(h,nullptr,FALSE);return 0;
 case WM_PAINT:{PAINTSTRUCT ps;BeginPaint(h,&ps);draw();EndPaint(h,&ps);return 0;}
 case WM_NCCALCSIZE:if(w)return 0;break;
 case WM_NCHITTEST:{
    LRESULT hit=DefWindowProcW(h,m,w,l);
    if(hit!=HTCLIENT)return hit;
    POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};ScreenToClient(h,&p);
    float x=float(p.x)/dpiScale,y=float(p.y)/dpiScale;
    if(!IsZoomed(h)){
        float edge=6;bool left=x<edge,right=x>cw()-edge,top=y<edge,bottom=y>ch()-edge;
        if(top&&left)return HTTOPLEFT;
        if(top&&right)return HTTOPRIGHT;
        if(bottom&&left)return HTBOTTOMLEFT;
        if(bottom&&right)return HTBOTTOMRIGHT;
        if(top)return HTTOP;
        if(bottom)return HTBOTTOM;
        if(left)return HTLEFT;
        if(right)return HTRIGHT;
    }
    if(y<88 && (x<236 || y<20 || (y>68&&x<cw()-205)))return HTCAPTION;
    return HTCLIENT;
 }
 case WM_MOUSEMOVE:{
    float x=float(GET_X_LPARAM(l))/dpiScale,y=float(GET_Y_LPARAM(l))/dpiScale;
    int next=hitControl(x,y),card=-1;
    if((page==0||page==1)&&y>=rowStart()&&y<ch()-45&&x>=leftEdge()){
        auto items=filteredApps();int row=int((y-rowStart()+catalogueScroll)/66);
        if(row>=0&&row<(int)items.size())card=items[row];
    }
    if(next!=hoverControl||card!=hoverCard){hoverControl=next;hoverCard=card;InvalidateRect(h,nullptr,FALSE);}
    SetCursor(LoadCursor(nullptr,next>=0?IDC_HAND:IDC_ARROW));
    TRACKMOUSEEVENT tracking{sizeof(TRACKMOUSEEVENT),TME_LEAVE,h,0};TrackMouseEvent(&tracking);return 0;
 }
 case WM_MOUSELEAVE:hoverControl=hoverCard=-1;InvalidateRect(h,nullptr,FALSE);return 0;
 case WM_CAPTURECHANGED:pressedControl=-1;drag=-1;InvalidateRect(h,nullptr,FALSE);return 0;
 case WM_CANCELMODE:pressedControl=-1;drag=-1;ReleaseCapture();InvalidateRect(h,nullptr,FALSE);return 0;
 case WM_MOUSEWHEEL:if(page==0||page==1){
    float visible=ch()-45-rowStart(),total=float(filteredApps().size())*66;
    catalogueScroll=std::clamp(catalogueScroll-float(GET_WHEEL_DELTA_WPARAM(w))/WHEEL_DELTA*66,0.f,std::max(0.f,total-visible));
    hoverControl=-1;InvalidateRect(h,nullptr,FALSE);
 }return 0;
 case WM_LBUTTONDOWN:{
    float x=float(GET_X_LPARAM(l))/dpiScale,y=float(GET_Y_LPARAM(l))/dpiScale;
    int target=hitControl(x,y);
    // SetCapture can synchronously send WM_CAPTURECHANGED, including to this window.
    // Store the new press only after that notification has cleared the previous one.
    if(target>=0)SetCapture(h);
    pressedControl=hoverControl=target;
    if(pressedControl>=projectBase&&pressedControl<projectBase+int(tiles.size())){
        selected=drag=pressedControl-projectBase;menuOpen=false;
    }
    if(pressedControl<0)menuOpen=false;
    InvalidateRect(h,nullptr,FALSE);return 0;
 }
 case WM_LBUTTONUP:{
    float x=float(GET_X_LPARAM(l))/dpiScale,y=float(GET_Y_LPARAM(l))/dpiScale;
    int released=hitControl(x,y),pressed=pressedControl,old=drag;
    pressedControl=-1;drag=-1;ReleaseCapture();hoverControl=released;
    if(old>=0&&released>=projectBase&&released<projectBase+int(tiles.size())){
        int target=released-projectBase;selected=target;
        if(target!=old){std::swap(tiles[old],tiles[target]);menuOpen=false;}
        else if(tiles[target].uses.size()>1)menuOpen=true;
        else openWizard(target,0);
    }else if(pressed>=0&&pressed==released){
        if(released>=navBase&&released<navBase+7){page=released-navBase;menuOpen=false;catalogueScroll=0;}
        else if(released==chromeBase)ShowWindow(h,SW_MINIMIZE);
        else if(released==chromeBase+1)ShowWindow(h,IsZoomed(h)?SW_RESTORE:SW_MAXIMIZE);
        else if(released==chromeBase+2){SendMessageW(h,WM_CLOSE,0,0);return 0;}
        else if(released==settingsControl){page=6;menuOpen=false;}
        else if(released>=filterBase&&released<filterBase+5){category=released-filterBase;catalogueScroll=0;}
        else if(released>=launchBase&&released<launchBase+3)status=L"Preview: connect application detection to enable Launch.";
        else if(released>=installBase&&released<installBase+5)status=L"Catalogue preview: installation is not connected yet.";
        else if(selected>=0&&released>=menuBase&&released<menuBase+int(tiles[selected].uses.size())){menuOpen=false;openWizard(selected,released-menuBase);}
    }
    InvalidateRect(h,nullptr,FALSE);return 0;
 }
 case WM_KEYDOWN:if(w==VK_ESCAPE&&menuOpen){menuOpen=false;InvalidateRect(h,nullptr,FALSE);}return 0;
 case WM_DESTROY:NativeIcons::shutdown();release(body);release(titleFormat);release(writeFactory);resetTarget();release(factory);PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,w,l);}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show){CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);enableDpiAwareness();WNDCLASSW wc{};wc.lpfnWndProc=wndProc;wc.hInstance=instance;wc.lpszClassName=L"MONOLITHToolboxMain";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);RegisterClassW(&wc);HWND h=CreateWindowExW(0,wc.lpszClassName,L"MONOLITH Toolbox",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1380,940,nullptr,nullptr,instance,nullptr);if(!h)return 1;BOOL dark=TRUE;DwmSetWindowAttribute(h,20,&dark,sizeof(dark));int corners=2;DwmSetWindowAttribute(h,33,&corners,sizeof(corners));SetWindowPos(h,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);ShowWindow(h,show);UpdateWindow(h);MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}CoUninitialize();return int(msg.wParam);}









