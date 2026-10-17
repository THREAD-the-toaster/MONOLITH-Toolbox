// Native Direct2D visual system. No bitmap background: every surface scales with DPI.
static int hoverCard=-1, category=0;
static float catalogueScroll=0;
static const wchar_t* navLabels[]={L"Home",L"Available",L"Installed",L"Start a project",L"Updates",L"Downloads",L"Settings"};

static float leftEdge(){return 264.f;}
static void line(float x,float y,float x2,float y2,D2D1_COLOR_F c,float width=1){
    ID2D1SolidColorBrush* b=nullptr;rt->CreateSolidColorBrush(c,&b);
    if(b){rt->DrawLine({x,y},{x2,y2},b,width);b->Release();}
}
static void gradient(float x,float y,float w,float h,float radius,unsigned top,unsigned bottom){
    D2D1_GRADIENT_STOP stops[]={{0,rgb(top)},{1,rgb(bottom)}};
    ID2D1GradientStopCollection* collection=nullptr;ID2D1LinearGradientBrush* brush=nullptr;
    rt->CreateGradientStopCollection(stops,2,&collection);
    if(collection){rt->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties({x,y},{x+w*.3f,y+h}),collection,&brush);
        if(brush){rt->FillRoundedRectangle(D2D1::RoundedRect(rect(x,y,w,h),radius,radius),brush);brush->Release();}collection->Release();}
}
static void textAt(const std::wstring& s,float x,float y,float w,float h,float size,unsigned color,bool bold=false,const wchar_t* font=L"Segoe UI"){
    IDWriteTextFormat* format=nullptr;ID2D1SolidColorBrush* brush=nullptr;
    writeFactory->CreateTextFormat(font,nullptr,bold?DWRITE_FONT_WEIGHT_SEMI_BOLD:DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,size,L"en-US",&format);
    rt->CreateSolidColorBrush(rgb(color),&brush);
    if(format&&brush){format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);DWRITE_TRIMMING trim{DWRITE_TRIMMING_GRANULARITY_CHARACTER,0,0};
        IDWriteInlineObject* ellipsis=nullptr;writeFactory->CreateEllipsisTrimmingSign(format,&ellipsis);format->SetTrimming(&trim,ellipsis);
        rt->DrawTextW(s.c_str(),UINT32(s.size()),format,rect(x,y,w,h),brush,D2D1_DRAW_TEXT_OPTIONS_CLIP);release(ellipsis);}
    release(format);release(brush);
}
static void glow(float x,float y,float w,float h,float radius,D2D1_COLOR_F accent){
    for(int i=6;i>=1;--i){
        float intensity=float(7-i)/6.f;
        stroke(x-i,y-i,w+2*i,h+2*i,radius+i,D2D1::ColorF(accent.r,accent.g,accent.b,.045f*intensity*intensity*accent.a),1.5f);
    }
}
#include "ui_controls.h"
static void appIcon(int app,float x,float y,float size){NativeIcons::draw(app,x,y,size);}
struct CatalogueApp {const wchar_t* name;const wchar_t* description;const wchar_t* version;const wchar_t* group;int icon;int category;};
static CatalogueApp catalogue[]={
    {L"Unreal Engine",L"Game engine and development tools",L"5.4.2",L"Game Development",3,1},
    {L"Godot Engine",L"Open source game engine",L"4.3",L"Game Development",4,1},
    {L"Python",L"Programming language",L"3.12.6",L"Development",5,1},
    {L"MONOLITH CAD Tools",L"3D modeling and CAD utilities",L"v0.8.2",L"Engineering",6,3},
    {L"CUDA Toolkit",L"GPU development tools",L"12.5",L"Development",7,1}
};
static const wchar_t* categories[]={L"All",L"Development",L"Design",L"Engineering",L"Utilities"};
static float menuTop(int i){auto r=tileRect(i);float height=float(tiles[i].uses.size()*44+14);return std::min(r.bottom-8,ch()-44-height);}
static float rowStart(){return page==0?480.f:240.f;}
static D2D1_RECT_F filterRect(int i){float width=(cw()-leftEdge()-28)/5;return rect(leftEdge()+i*width, rowStart()-52,width-6,40);}
static std::vector<int> filteredApps(){std::vector<int> out;for(int i=0;i<5;i++)if(category==0||catalogue[i].category==category)out.push_back(i);return out;}
static void catalogueView(){
    float X=leftEdge(),W=cw()-X-28,top=rowStart();
    textAt(L"Available",X,top-116,W,35,25,0xf2f5fa,true);
    textAt(L"Browse tools from GitHub and GitLab.",X,top-80,W,22,14,0xa5b7cf);
    for(int i=0;i<5;i++)uiButton(filterRect(i),categories[i],filterBase+i,rgb(0x489eff),false,-1,true,category==i);
    float bottom=ch()-45;
    auto savedClip=controlClip;controlClip=rect(X,top,W,std::max(0.f,bottom-top));
    rt->PushAxisAlignedClip(controlClip,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    auto items=filteredApps();
    for(int k=0;k<(int)items.size();k++){
        int i=items[k];const auto& a=catalogue[i];float y=top+k*66-catalogueScroll;
        if(y+66<top||y>=bottom)continue;
        gradient(X,y,W,66, k==0?12:0,hoverCard==i?0x182737:0x111920,0x090f15);
        line(X+18,y+65,X+W-18,y+65,rgb(0x26313d,.65f));appIcon(a.icon,X+16,y+10,44);
        float controls=170,meta=W>790?190.f:126.f;float nameWidth=W-98-controls-meta;
        textAt(a.name,X+86,y+10,nameWidth,24,16,0xecf2f9,true);
        textAt(a.description,X+86,y+36,nameWidth,23,13,0x9db0c9);
        textAt(a.version,X+W-controls-meta,y+12,meta-8,22,14,0xa9bed8);
        textAt(a.group,X+W-controls-meta,y+37,meta-8,22,12,0x8dabc8);
        uiButton(rect(X+W-161,y+13,111,actionHeight),L"Install",installBase+i,applicationAccent(a.icon),true,NativeIcons::NavDownloads);
        uiButton(rect(X+W-44,y+13,36,actionHeight),L"",expandBase+i,applicationAccent(a.icon),false,NativeIcons::UiChevron,false);
    }
    if(items.empty())textAt(L"No tools in this category yet.",X+20,top+25,W-40,30,15,0x9db0c9);
    rt->PopAxisAlignedClip();controlClip=savedClip;
    float total=float(items.size())*66,viewport=bottom-top;
    if(total>viewport&&viewport>0){float thumb=viewport*viewport/total;rounded(cw()-18,top+catalogueScroll/total*viewport,3,thumb,2,rgb(0x52759c));}
}
static void installedView(){
    float X=leftEdge(),W=cw()-X-28;
    textAt(L"Installed",X,112,W,36,26,0xf3f6fc,true);
    textAt(L"Your tools, ready to launch.",X,148,W-200,26,15,0xa5b8d1);
    uiButton(rect(cw()-173,125,58,actionHeight),L"",700,rgb(0x489eff),false,NativeIcons::UiGrid,false);
    uiButton(rect(cw()-106,125,78,actionHeight),L"Name",701,rgb(0x489eff),false,-1,false);
    const wchar_t* names[]={L"Visual Studio Code",L"Blender",L"Git"};const wchar_t* versions[]={L"1.93.1",L"4.2.1",L"2.46.0"};
    float width=(W-32)/3;
    for(int i=0;i<3;i++){float x=X+i*(width+16);gradient(x,190,width,158,12,0x19232d,0x0a1017);stroke(x,190,width,158,12,rgb(0x2b3744,.8f));
        appIcon(i,x+18,211,49);float nameX=x+80;textAt(names[i],nameX,207,width-98,26,15,0xf0f4fa,true);
        textAt(versions[i],nameX,233,width-98,22,13,0xa0b6d0);
        rounded(nameX,262,7,7,4,rgb(0x06dfb0));textAt(L"Preview",nameX+14,256,width-112,22,13,0x9fb3cc);
        uiButton(rect(x+20,293,width-40,actionHeight),L"Launch",launchBase+i,applicationAccent(i),true,NativeIcons::UiPlay);}
}
static void draw(){
    initTarget();if(!rt||!writeFactory)return;
    rt->BeginDraw();rt->SetTransform(D2D1::Matrix3x2F::Scale(dpiScale,dpiScale));rt->Clear(rgb(0x080d13));
    uiControls.clear();
    float W=cw(),H=ch();controlClip=rect(0,0,W,H);gradient(0,0,W,H,0,0x101a26,0x060b10);
    gradient(0,0,W,88,0,0x202c3a,0x0b1119);line(0,87,W,87,rgb(0x8298b3,.14f));
    gradient(0,88,236,H-88,0,0x111d2b,0x080e17);line(236,88,236,H,rgb(0x5d7b9d,.18f));
    NativeIcons::draw(NativeIcons::Brand,25,22,44);textAt(L"M O N O L I T H",85,24,145,25,16,0xc0d0e6,true);textAt(L"T O O L B O X",86,51,142,20,10,0x90a9c8);
    gradient(256,23,W-468,43,12,0x090f18,0x0c141e);stroke(256,23,W-468,43,12,rgb(0x3b4c60,.7f));
    NativeIcons::draw(NativeIcons::UiSearch,272,35,19);textAt(L"Search tools, frameworks, or repositories...",304,35,W-528,24,13,0x819dbf);
    uiButton(rect(W-198,25,38,actionHeight),L"",settingsControl,rgb(0x489eff),false,NativeIcons::NavSettings);
    uiButton(rect(W-145,25,41,actionHeight),L"",chromeBase,rgb(0x489eff),false,NativeIcons::UiMinimize);
    uiButton(rect(W-98,25,41,actionHeight),L"",chromeBase+1,rgb(0x489eff),false,IsZoomed(windowHandle)?NativeIcons::UiRestore:NativeIcons::UiMaximize);
    uiButton(rect(W-51,25,41,actionHeight),L"",chromeBase+2,rgb(0xee5260),false,NativeIcons::UiClose);
    for(int i=0;i<7;i++){
        float y=104+i*55.f;bool active=page==i;int id=navBase+i;
        bool hover=hoverControl==id,pressed=hover&&pressedControl==id;
        registerControl(id,rect(16,y,204,48));
        if(active){
            glow(16,y,204,48,12,D2D1::ColorF(.12f,.56f,1.f,hover?.8f:.45f));
            gradient(16,y,204,48,12,pressed?0x122b47:hover?0x34668f:0x23496e,pressed?0x102137:0x102039);
            stroke(16,y,204,48,12,rgb(hover?0x8ed2ff:0x559dd0,pressed?.55f:.8f));
            line(219,y+12,219,y+36,rgb(0x69c9ff,pressed?.4f:.8f),1.3f);
        }else{
            rounded(16,y,204,48,12,rgb(pressed?0x09131f:hover?0x1b2d41:0x101b29));
            stroke(16,y,204,48,12,rgb(hover?0x3e5c7d:0x253447,.65f));
        }
        NativeIcons::draw(NativeIcons::NavHome+i,34,y+13+(pressed?1:0),22,active||hover?1.f:.8f);
        textAt(navLabels[i],76,y+13+(pressed?1:0),136,26,14,active?0xf5f8ff:0xa9bdd5);
    }
    gradient(16,H-86,204,70,13,0x101e2a,0x080f18);rounded(32,H-56,12,12,6,rgb(0x20e3dc));glow(32,H-56,12,12,6,rgb(0x20e3dc));
    textAt(L"Ready",57,H-65,142,25,14,0x50e5df);textAt(L"Network idle",57,H-41,145,22,11,0x9cb0c9);
    rt->PushAxisAlignedClip(rect(237,89,W-237,H-89),D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    if(page==0){installedView();catalogueView();}
    else if(page==1)catalogueView();
    else if(page==2)installedView();
    else if(page==3){textAt(L"Start a quick project",260,112,W-288,43,26,0xf2f6fc,true);textAt(L"A local workspace for your next idea.",260,156,W-288,27,15,0xa3b9d3);
        for(int i=0;i<(int)tiles.size();i++){auto r=tileRect(i);float tw=r.right-r.left;gradient(r.left,r.top,tw,r.bottom-r.top,12,hoverControl==projectBase+i?0x25394e:0x1b2837,0x0b121c);stroke(r.left,r.top,tw,r.bottom-r.top,12,selected==i?tiles[i].accent:rgb(0x304357));
            NativeIcons::draw(NativeIcons::projectIcon(tiles[i].name),r.left+17,r.top+16,36);
            textAt(tiles[i].name,r.left+65,r.top+22,tw-80,25,15,0xf2f6ff,true);textAt(tiles[i].subtitle,r.left+18,r.top+64,tw-36,25,14,0x9fb4ce);
            registerControl(projectBase+i,r);
            uiButton(rect(r.left+16,r.top+100,tw-32,actionHeight),tiles[i].uses.size()>1?L"Choose type":L"New project",projectBase+i,tiles[i].accent,true,tiles[i].uses.size()>1?NativeIcons::UiChevron:NativeIcons::NavProject,true,false,false);}
        if(selected>=0&&menuOpen){
            auto r=tileRect(selected);float y=menuTop(selected),h=float(tiles[selected].uses.size()*44+14);
            gradient(r.left+12,y,210,h,12,0x20344c,0x101b29);stroke(r.left+12,y,210,h,12,rgb(0x52789e));
            for(int k=0;k<(int)tiles[selected].uses.size();k++)uiButton(rect(r.left+17,y+7+k*44,200,40),tiles[selected].uses[k],menuBase+k,tiles[selected].accent,false);
        }
    }else if(page==6){textAt(L"Settings",260,112,W-288,40,26,0xf2f6fc,true);textAt(L"Applications and project templates",260,158,W-288,27,15,0x9fb7d2);
        for(int i=0;i<(int)tiles.size();i++){float y=208+i*58.f;gradient(260,y,W-288,49,10,0x192838,0x0c1621);textAt(tiles[i].name,278,y+13,W-510,26,14,0xe8f3ff);textAt(L"Template enabled",W-195,y+13,165,26,13,0x7ddfbc);}}
    else {textAt(navLabels[page],260,112,W-288,40,26,0xf2f6fc,true);textAt(L"Nothing here yet. You're all set.",260,169,W-288,30,15,0x9fb7d2);}
    // The catalogue reproduces the reference layout; integration remains a prototype.
    gradient(245,H-32,W-253,28,5,0x0c1721,0x0a1119);textAt(status==L"Ready  |  Network idle"?L"PREVIEW  /  Sample catalogue - app detection and installation are not connected":status,260,H-26,W-286,22,11,0x7f9bb9);
    rt->PopAxisAlignedClip();stroke(.5f,.5f,W-1,H-1,0,rgb(0x526a83,.65f));
    HRESULT hr=rt->EndDraw();if(hr==D2DERR_RECREATE_TARGET)resetTarget();
}





