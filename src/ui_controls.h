#pragma once
// Geometry is shared by drawing and input; clipped rows cannot steal clicks.
struct UiControl {int id;D2D1_RECT_F bounds;bool enabled;};
static std::vector<UiControl> uiControls;
static D2D1_RECT_F controlClip{0,0,100000,100000};
static int hoverControl=-1,pressedControl=-1;
static constexpr float actionHeight=40.f,cornerRadius=11.f;
static constexpr int navBase=10,chromeBase=20,settingsControl=23,filterBase=100,
    launchBase=200,installBase=300,expandBase=400,projectBase=500,menuBase=600;
static void registerControl(int id,D2D1_RECT_F bounds,bool enabled=true){
    bounds.left=std::max(bounds.left,controlClip.left);bounds.top=std::max(bounds.top,controlClip.top);
    bounds.right=std::min(bounds.right,controlClip.right);bounds.bottom=std::min(bounds.bottom,controlClip.bottom);
    if(bounds.right>bounds.left&&bounds.bottom>bounds.top)uiControls.push_back({id,bounds,enabled});
}
static int hitControl(float x,float y){
    for(auto it=uiControls.rbegin();it!=uiControls.rend();++it)if(inside(x,y,it->bounds))return it->enabled?it->id:-1;
    return -1;
}
static unsigned mixColor(unsigned base,D2D1_COLOR_F accent,float amount){
    auto c=rgb(base);unsigned r=unsigned((c.r*(1-amount)+accent.r*amount)*255+.5f);
    unsigned g=unsigned((c.g*(1-amount)+accent.g*amount)*255+.5f),b=unsigned((c.b*(1-amount)+accent.b*amount)*255+.5f);
    return (r<<16)|(g<<8)|b;
}
static D2D1_COLOR_F applicationAccent(int app){
    const unsigned colors[]={0x29a9ff,0xff921e,0xf04b43,0xa879ff,0x4db9e8,0xffcf45,0x55a9f4,0x80c928};
    return rgb(app>=0&&app<8?colors[app]:0x5aabff);
}
static void uiButton(D2D1_RECT_F r,const std::wstring& label,int id,D2D1_COLOR_F accent,
                     bool primary=true,int icon=-1,bool enabled=true,bool active=false,bool registerHit=true){
    if(registerHit)registerControl(id,r,enabled);
    bool hover=enabled&&hoverControl==id,pressed=hover&&pressedControl==id;
    float w=r.right-r.left,h=r.bottom-r.top;
    if(!enabled){
        rounded(r.left,r.top,w,h,cornerRadius,rgb(0x151c25));stroke(r.left,r.top,w,h,cornerRadius,rgb(0x2d3541));
    }else if(primary){
        if(!pressed)glow(r.left,r.top,w,h,cornerRadius,D2D1::ColorF(accent.r,accent.g,accent.b,hover?.9f:.4f));
        gradient(r.left,r.top,w,h,cornerRadius,mixColor(pressed?0x0b1420:hover?0x35495e:0x253446,accent,pressed?.10f:hover?.24f:.12f),mixColor(pressed?0x152233:0x101925,accent,.06f));
        stroke(r.left,r.top,w,h,cornerRadius,D2D1::ColorF(accent.r,accent.g,accent.b,pressed?.5f:hover?.95f:.65f));
        line(r.left+12,r.top+(pressed?2:1),r.right-12,r.top+(pressed?2:1),rgb(pressed?0x000000:0xe1efff,pressed?.45f:hover?.5f:.25f));
        if(!pressed)line(r.right-42,r.bottom-1,r.right-18,r.bottom-1,D2D1::ColorF(accent.r,accent.g,accent.b,hover?.95f:.65f),1.3f);
    }else{
        // Secondary controls are matte; only the active selection has a blue tint.
        rounded(r.left,r.top,w,h,cornerRadius,rgb(pressed?0x0a121d:active?0x142b46:hover?0x1c2c40:0x101b29));
        stroke(r.left,r.top,w,h,cornerRadius,rgb(active?0x427eb1:hover?0x45617e:0x29384b));
        if(pressed)line(r.left+12,r.top+1,r.right-12,r.top+1,rgb(0x000000,.5f));
    }
    float offset=pressed?1.f:0.f;
    if(icon>=0)NativeIcons::draw(icon,r.left+(label.empty()?(w-19)*.5f:13),r.top+(h-19)*.5f+offset,19,enabled?1.f:.35f);
    if(!label.empty())textAt(label,r.left+(icon>=0?40:13),r.top+(h-21)*.5f+offset,w-(icon>=0?48:26),25,14,enabled?0xe7f1ff:0x707c8d,primary);
}
