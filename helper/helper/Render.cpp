#include "Render.h"
#include <thread>

LRESULT CALLBACK msgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
HWND createWindow(const char* winTitle, UINT width, UINT height);

void Render::cameraUpdate(InputEngine input) {
  camera.update(input);

  projecton_matrix = XMMatrixPerspectiveFovLH(
      fovy * DEG2RAD, float(renderWidth) / renderHeight, 0.1f, 5000.0f);

  float3 cPos = camera.getCameraPos();
  float3 cZdir = camera.getCameraZ();
  XMVECTOR pos = XMVectorSet(cPos.x, cPos.y, cPos.z, 1.0f);
  XMVECTOR target =
      XMVectorSet(cPos.x + cZdir.x, cPos.y + cZdir.y, cPos.z + cZdir.z, 1.0f);
  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  camera_maxtirx = XMMatrixLookAtLH(pos, target, up);

  vp_matrix = camera_maxtirx * projecton_matrix;
}

void Render::init() {
  if (!hwnd) {
    hwnd = createWindow("rendering", renderWidth, renderHeight);
    ShowWindow(hwnd, SW_SHOW);

    swapChain.create(hwnd, &rtvHeap, &cmdqueue, renderWidth, renderHeight, 2,
                     DXGI_FORMAT_R8G8B8A8_UNORM);
  }

  InputEngine input(hwnd);

  camera.setFovY(fovy);
  camera.setScreenSize((float)renderWidth, (float)renderHeight);
  camera.initOrbit(float3(0.0f, 160.0f, 0.0f), 100.0f, 0.0f, 0.0f);

  MeshData mesh{&cmdqueue, &cmdlist, "./data/metahuman.obj", 0, 0, 0, true,
                false,     false};
  DepthTarget depth{&srvHeap,    &dsvHeap,    &cmdqueue, DXGI_FORMAT_D32_FLOAT,
                    renderWidth, renderHeight};
  Texture skin{&srvHeap, &cmdqueue, DXGI_FORMAT_R8G8B8A8_UNORM,
               "./data/FaceBaseColor.png"};


  mdPass.setTargetSize(renderWidth, renderHeight);
  mdPass.bindRenderTarget(swapChain.getRtv());
  mdPass.bindDepthTarget(depth);

  rectlight.setTargetSize(renderWidth, renderHeight);
  rectlight.bindRenderTarget(swapChain.getRtv());
  rectlight.bindDepthTarget(depth);

  float3 light_position(20, 150, 50);
  XMMATRIX translate =
      XMMatrixTranslation(light_position.x, light_position.y, light_position.z);
  XMMATRIX scale = XMMatrixScaling(15, 10, 1.0f);
  rect_matrix = scale * translate;

  XMMATRIX translate2 = XMMatrixTranslation(10, 0, 20);
  XMMATRIX translate3 = XMMatrixTranslation(0, 0, 0);


  tsPass.bind("diffuseColor", skin.getSrv());
  tsPass.bindRenderTarget(target[0], 0);
  tsPass.bindRenderTarget(target[1], 1);
  tsPass.bindRenderTarget(target[2], 2);
  tsPass.setTargetSize(imageW, imageH);
  tsPass.render(&cmdqueue, &cmdlist,
                TextureSpace::RenderInfo{mesh.renderInfo.vtxBuffView,
                                         mesh.renderInfo.idxBuffView,
                                         mesh.renderInfo.numTriangles});


  lightPass.bind("diffuse", target[0].getSrv());
  lightPass.bind("position", target[1].getSrv());
  lightPass.bind("normal", target[2].getSrv());
  lightPass.bindRenderTarget(lightTarget);
  lightPass.setTargetSize(imageW, imageH);

  mdPass.bind("shadedColor", lightTarget.getSrv());
  
  float intensity = 2000;

  while (IsWindow(hwnd)) {
    input.update();

    cameraUpdate(input);

    clearTargets(cmdqueue, cmdlist, {&swapChain.getRtv()}, {&depth.getDsv()});

    tsPass.bind("modelMat", {translate3});
    tsPass.render(&cmdqueue, &cmdlist,
                  TextureSpace::RenderInfo{mesh.renderInfo.vtxBuffView,
                                           mesh.renderInfo.idxBuffView,
                                           mesh.renderInfo.numTriangles});

    lightPass.bind("data", {float4(light_position, 1.0), float4(0, 0, -1, 1),
                            camera.getCameraPos(), intensity});
    lightPass.render(&cmdqueue, &cmdlist);

    mdPass.bind("viewData", {translate3 * vp_matrix});
    mdPass.render(&cmdqueue, &cmdlist,
                  MeshDraw::RenderInfo{mesh.renderInfo.vtxBuffView,
                                       mesh.renderInfo.idxBuffView,
                                       mesh.renderInfo.numTriangles});

    tsPass.bind("modelMat", {translate2});
    tsPass.render(&cmdqueue, &cmdlist,
                  TextureSpace::RenderInfo{mesh.renderInfo.vtxBuffView,
                                           mesh.renderInfo.idxBuffView,
                                           mesh.renderInfo.numTriangles});

    lightPass.bind("data", {float4(light_position, 1.0), float4(0, 0, -1, 1),
                            camera.getCameraPos(), intensity});
    lightPass.render(&cmdqueue, &cmdlist);

    mdPass.bind("viewData", {translate2 * vp_matrix});
    mdPass.render(&cmdqueue, &cmdlist,
                  MeshDraw::RenderInfo{mesh.renderInfo.vtxBuffView,
                                       mesh.renderInfo.idxBuffView,
                                       mesh.renderInfo.numTriangles});


    rectlight.bind("viewData",
                   {rect_matrix * vp_matrix, float4(1.0f, 1.0f, 1.0f, 1.0f)});
    rectlight.render(&cmdqueue, &cmdlist);

    swapChain.present();
    cmdqueue.waitCompletion();
    cmdlist.reset();

    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  cmdqueue.waitCompletion();
  cmdqueue.signalForSafety();
}

LRESULT CALLBACK msgProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  switch (message) {
    case WM_SIZE:

      return 0;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND createWindow(const char* winTitle, UINT width, UINT height) {
  WNDCLASSA wc = {};
  wc.lpfnWndProc = msgProc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.lpszClassName = "anything";
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  RegisterClassA(&wc);

  RECT r{0, 0, (LONG)width, (LONG)height};
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);

  HWND hWnd =
      CreateWindowA(wc.lpszClassName, winTitle, WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left,
                    r.bottom - r.top, nullptr, nullptr, wc.hInstance, nullptr);

  return hWnd;
}