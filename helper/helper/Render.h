#pragma once
#include "Helper.h"
#include "Input.h"
#include "Camera.h"
#include "Pass.h"



class Render {
 private:
  DescriptorHeap rtvHeap{128, D3D12_DESCRIPTOR_HEAP_TYPE_RTV};
  DescriptorHeap srvHeap{256, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV};  
  DescriptorHeap dsvHeap{32, D3D12_DESCRIPTOR_HEAP_TYPE_DSV};
  CommandQueue cmdqueue{D3D12_COMMAND_LIST_TYPE_DIRECT};
  CommandList cmdlist{D3D12_COMMAND_LIST_TYPE_DIRECT};

  HWND hwnd = nullptr;
  UINT renderWidth = 1200;
  UINT renderHeight = 900;
  UINT imageW = 4096;
  UINT imageH = 4096;

  SwapChain swapChain;
  OrbitCamera camera;
  float fovy = 30.0f;
  XMMATRIX projecton_matrix;
  XMMATRIX camera_maxtirx;
  XMMATRIX vp_matrix;
  XMMATRIX rect_matrix;

  Pass<MeshDraw> mdPass{&srvHeap};
  Pass<RectDraw> rectlight{&srvHeap};
  Pass<TextureSpace> tsPass{&srvHeap};
  Pass<LightSpace> lightPass{&srvHeap};

  RenderTarget target[3]{{&srvHeap, &rtvHeap, &cmdqueue,
                            DXGI_FORMAT_R32G32B32A32_FLOAT, imageW, imageH},
                             {&srvHeap, &rtvHeap, &cmdqueue,
                              DXGI_FORMAT_R32G32B32A32_FLOAT, imageW, imageH},
                             {&srvHeap, &rtvHeap, &cmdqueue,
                              DXGI_FORMAT_R32G32B32A32_FLOAT, imageW, imageH}};
  RenderTarget lightTarget{&srvHeap,  &rtvHeap,
                           &cmdqueue, DXGI_FORMAT_R32G32B32A32_FLOAT,
                           imageW,    imageH};

 public:
  void init();
  void cameraUpdate(InputEngine input);
};
