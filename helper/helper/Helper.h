#pragma once
#include <iostream>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>
#include <variant>
#include <typeindex>
#include <map>
#include <any>
#include <set>

#include "basic_types.h"



#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxcompiler.lib")

using namespace DirectX;
#define SAFE_DELETE(x) if (x) delete x; x = nullptr
#define SAFE_DELETE_ARR(x) if (x) delete[] x; x = nullptr
#define SAFE_RELEASE(x)  if (x) (x)->Release(); x = nullptr

class CommandQueue;
class CommandList;

enum class DescriptorType { SRV, UAV, CBV, RTV, DSV, Sampler };
enum DepthMode { depth_disable, depth_readOnly, depth_enable };
enum BlendMode {
  blend_opaque,
  blend_translucent,
  blend_additive,
};

inline void ThrowFailedHR(HRESULT hr) {
  if (FAILED(hr)) {
    throw hr;
  }
}

inline void Error(const char* errorMessage) {
  if (errorMessage) {
    throw puts(errorMessage);
  }
}

inline D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource,
                                         D3D12_RESOURCE_STATES stateBefore,
                                         D3D12_RESOURCE_STATES stateAfter) {
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = resource;
  barrier.Transition.StateBefore = stateBefore;
  barrier.Transition.StateAfter = stateAfter;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  return barrier;
}

inline constexpr UINT _bpp(DXGI_FORMAT format) {  // bytes per pixel
  switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_FLOAT:
      return 4;

    case DXGI_FORMAT_R32G32B32_FLOAT:
      return 12;

    case DXGI_FORMAT_R16G16B16A16_UNORM:
      return 8;

    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
      return 16;

    default:
      assert(false);
      return UINT(-1);
  }
}

inline constexpr UINT _channels(DXGI_FORMAT format) {
  switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
      return 4;

    case DXGI_FORMAT_R32G32B32_FLOAT:
      return 3;

    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_FLOAT:
      return 1;

    default:
      assert(false);
      return UINT(-1);
  }
}

template <typename UnsignedType, typename PowerOfTwo>
inline constexpr UnsignedType _align(UnsignedType val, PowerOfTwo base) {
  //base unit clamp
  return (val + (UnsignedType)base - 1) & ~((UnsignedType)base - 1);
}

ID3D12Resource* createCommittedTexture(
    DXGI_FORMAT format, UINT width, UINT height, UINT depth = 1,
    D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE,
    D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_COMMON,
    D3D12_CLEAR_VALUE* pOptClearValue = nullptr);

ID3D12Resource* createCommittedBuffer(
    UINT64 bufferSize, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD,
    D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE,
    D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_GENERIC_READ);

void clearTargets(CommandQueue& cmdqueue, CommandList& cmdList,
                  const std::vector<const class Descriptor*>& rtvs,
                  const std::vector<const class Descriptor*>& dsvs);

inline D3D12_RANGE Range(SIZE_T end, SIZE_T begin = 0) {
  D3D12_RANGE range;
  range.Begin = begin;
  range.End = end;
  return range;
}

inline UINT findCommaOrNull(const char* codePos) {
  UINT i = 0;
  for (; codePos[i] != '\0'; ++i) {
    if (codePos[i] == ',') break;
  }
  return i;
}

inline UINT spaceRemoveCopy(char* dst, const char* src, UINT length) {
  UINT j = 0;
  for (UINT i = 0; i < length; ++i) {
    if (src[i] == ' ' || src[i] == '\t') continue;
    dst[j++] = src[i];
  }
  dst[j] = '\0';
  return j;
}

inline UINT readDigit(const char* digitPos, UINT* num) {
  *num = 0;

  UINT i = 0;
  for (; digitPos[i] != '\0'; ++i) {
    if ('0' <= digitPos[i] && digitPos[i] <= '9') {
      *num *= 10;
      *num += (UINT)(digitPos[i] - '0');
    } else {
      if (i == 0) Error("At least one digit must be placed even zero");
      break;
    }
  }

  return i;
}



class Device;
inline Device* getDevice();

class Device {
 private:
  ID3D12DebugDevice* debug = nullptr;
  ID3D12Device* device = nullptr;
  IDXGIFactory2* factory = nullptr;
  IDXGIAdapter* adapter = nullptr;
 public:
  Device() {
#ifdef _DEBUG
    ID3D12Debug1* debuger;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debuger))))
      debuger->EnableDebugLayer();
#endif
    ThrowFailedHR(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
    if (!factory) Error("failed to create a factory...");

    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters(i, &adapter);
         i++) {
      ID3D12Device* tempDevice;
      ThrowFailedHR(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0,
                                      IID_PPV_ARGS(&tempDevice)));

      D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5;
      HRESULT hr = tempDevice->CheckFeatureSupport(
          D3D12_FEATURE_D3D12_OPTIONS5, &features5,
          sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));

      if (FAILED(hr)) Error("failed to create a device...");

      // RAYTRACING 
      //if (features5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
      //  SAFE_RELEASE(tempDevice);
      //  break;
      //}
      if (adapter) {
        SAFE_RELEASE(tempDevice);
        break;
      }
    }

    if (!adapter) Error("failed to create a adapter...");

    ThrowFailedHR(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0,
                                    IID_PPV_ARGS(&device)));
#ifdef _DEBUG
    ThrowFailedHR(device->QueryInterface(IID_PPV_ARGS(&debug)));
#endif
  }
  ~Device() {
    SAFE_RELEASE(adapter);
    SAFE_RELEASE(factory);
    SAFE_RELEASE(device);
    SAFE_RELEASE(debug);
  }

  ID3D12DebugDevice* getDebugDevice() { return debug; }
  ID3D12Device* get() { return device; }
  IDXGIFactory2* getFactory() { return factory; }
  IDXGIAdapter* getAdapter() { return adapter; }
};
inline Device* getDevice() {
  static std::unique_ptr<Device> singleton = nullptr;
  if (!singleton) {
    singleton = std::make_unique<Device>();
  }
  return singleton.get();
}

class CommandQueue {
  D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  ID3D12CommandQueue* cmdQueue = nullptr;
  ID3D12Fence* fence = nullptr;
  UINT64 fenceValue = 0;

 public:
  ID3D12CommandQueue* get() { return cmdQueue; }
  D3D12_COMMAND_LIST_TYPE getType() { return type; }
  UINT64 excuteCommandList(ID3D12CommandList* rawList);
  ~CommandQueue();
  explicit CommandQueue(
      D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
  void waitCompletion(UINT64 targetValue = 0, HANDLE handle = nullptr);
  void signalForSafety();
};

class CommandList {
  D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  ID3D12GraphicsCommandList* cmdList = nullptr;
  ID3D12CommandAllocator* cmdAllocator = nullptr;

 public:
  ID3D12GraphicsCommandList* get() { return cmdList; }
  ID3D12CommandAllocator* getAllocator() { return cmdAllocator; }
  D3D12_COMMAND_LIST_TYPE getType() { return type; }
  ~CommandList();
  explicit CommandList(
      D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
  void reset();
  ID3D12GraphicsCommandList* begin();
  UINT64 end(CommandQueue* queue, bool waitFinish = false);
};

class dxResource {
 protected:
  ID3D12Resource* resource = nullptr;
  mutable D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

  UINT64 bufferSize = 0;
  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;

 public:
  virtual ~dxResource() {
    partialDestroy();
    destroy();
  }
  void partialDestroy() {
    bufferSize = 0;
    gpuAddress = 0;
  }
  void destroy() {
    SAFE_RELEASE(resource);
    resourceState = D3D12_RESOURCE_STATE_COMMON;
  }
  ID3D12Resource* get() const { return resource; }
  D3D12_RESOURCE_STATES getState() const { return resourceState; }
  D3D12_RESOURCE_STATES changeResourceState(
      ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState) const;
};

class DxBuffer : public dxResource {
 public:
  // cpu : cpu has the data and puts the memory in the instruction queue
  // gpu : upload memory directly to gpu (upload resource is required)
  enum class StorageType { cpu, gpu };

 private:
  UINT64 bufferSize = 0;
  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;

  const StorageType type;
  ID3D12Resource* uploader = nullptr;
  void* cpuAddress = nullptr;

 public:
  ~DxBuffer() { partialDestroy(); }
  void destroy() {
    partialDestroy();
    dxResource::destroy();
  }
  void partialDestroy() {
    bufferSize = 0;
    gpuAddress = 0;
    cpuAddress = nullptr;
  }

  explicit DxBuffer(StorageType type = StorageType::cpu) : type(type) {}
  explicit DxBuffer(UINT64 bufferSize, StorageType type = StorageType::cpu)
      : type(type) {
    create(bufferSize);
  }
  void create(UINT64 bufferSize, D3D12_HEAP_TYPE heapType,
              D3D12_RESOURCE_FLAGS rscFlag, D3D12_RESOURCE_STATES rscState);
  void create(UINT64 bufferSize) {
    if (type == StorageType::cpu)
      create(bufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE,
             D3D12_RESOURCE_STATE_GENERIC_READ);
    else
      create(bufferSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE,
             D3D12_RESOURCE_STATE_COMMON);
  }
  UINT64 getGpuAddress() const {
    if (type == StorageType::gpu) {
      assert(!cpuAddress);
    }
    return gpuAddress;
  }

  UINT64 getBufferSize() const { return bufferSize; }

  void* map(); // memory copy
  void unmap(CommandQueue* cmdQueue, CommandList* cmdList); //release memory, wait for gpu until memory is released
};

class Descriptor {
  // create a resource descriptor
  mutable const dxResource* resource = nullptr;
  D3D12_CPU_DESCRIPTOR_HANDLE cpuAddr{};
  D3D12_GPU_DESCRIPTOR_HANDLE gpuAddr{};

  friend class DescriptorHeap;
  friend class SwapChain;
  Descriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuAddr,
             D3D12_GPU_DESCRIPTOR_HANDLE gpuAddr)
      : cpuAddr(cpuAddr), gpuAddr(gpuAddr) {}
  Descriptor() = default;
  Descriptor& operator=(const Descriptor&) = default;

 public:
  const dxResource* getResource() const { return resource; }
  const D3D12_CPU_DESCRIPTOR_HANDLE& getCpuHandle() const { return cpuAddr; }
  const D3D12_GPU_DESCRIPTOR_HANDLE& getGpuHandle() const { return gpuAddr; }
  operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return cpuAddr; }
  operator D3D12_GPU_DESCRIPTOR_HANDLE() const { return gpuAddr; }

  void assignRtv(const dxResource& resource,
                 const D3D12_RENDER_TARGET_VIEW_DESC* desc = nullptr) const {
    getDevice()->get()->CreateRenderTargetView(resource.get(), desc,
                                                     cpuAddr);
    this->resource = &resource;
  }
  void assignDsv(const dxResource& resource,
                 const D3D12_DEPTH_STENCIL_VIEW_DESC* desc = nullptr) const {
    getDevice()->get()->CreateDepthStencilView(resource.get(), desc,
                                                     cpuAddr);
    this->resource = &resource;
  }
  void assignSrv(const dxResource& resource,
                 const D3D12_SHADER_RESOURCE_VIEW_DESC* desc = nullptr) const {
    getDevice()->get()->CreateShaderResourceView(resource.get(), desc,
                                                       cpuAddr);
    this->resource = &resource;
  }
  void assignUav(const dxResource& resource,
                 const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc = nullptr,
                 ID3D12Resource* counter = nullptr) const {
    getDevice()->get()->CreateUnorderedAccessView(resource.get(), counter,
                                                        desc, cpuAddr);
    this->resource = &resource;
  }
  void assignCbv(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc) const {
    getDevice()->get()->CreateConstantBufferView(desc, cpuAddr);
  }
};

class DescriptorHeap {
  //  create a descriptor storage space
  //  free up resource memory space in cpu and gpu
  D3D12_DESCRIPTOR_HEAP_TYPE type;
  ID3D12DescriptorHeap* heap = nullptr;
  std::vector<Descriptor> descriptorArr;
  UINT numAssigned = 0;

 public:
  ~DescriptorHeap();
  DescriptorHeap() = delete;
  DescriptorHeap(UINT maxDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE heapType);

  ID3D12DescriptorHeap* get() const { return heap; }
  void bind(ID3D12GraphicsCommandList* cmdList) const;

  const Descriptor* assignRtv(
      const dxResource& resource,
      const D3D12_RENDER_TARGET_VIEW_DESC* desc = nullptr) {
    assert(type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    descriptorArr[numAssigned].assignRtv(resource, desc);
    return &descriptorArr[numAssigned++];
  }
  const Descriptor* assignDsv(
      const dxResource& resource,
      const D3D12_DEPTH_STENCIL_VIEW_DESC* desc = nullptr) {
    assert(type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    descriptorArr[numAssigned].assignDsv(resource, desc);
    return &descriptorArr[numAssigned++];
  }
  const Descriptor* assignSrv(
      const dxResource& resource,
      const D3D12_SHADER_RESOURCE_VIEW_DESC* desc = nullptr) {
    assert(type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    descriptorArr[numAssigned].assignSrv(resource, desc);
    return &descriptorArr[numAssigned++];
  }
  const Descriptor* assignUav(
      const dxResource& resource,
      const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc = nullptr,
      ID3D12Resource* counter = nullptr) {
    assert(type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    descriptorArr[numAssigned].assignUav(resource, desc, counter);
    return &descriptorArr[numAssigned++];
  }
  const Descriptor* assignCbv(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc) {
    assert(type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    descriptorArr[numAssigned].assignCbv(desc);
    return &descriptorArr[numAssigned++];
  }
};

class Texture : public dxResource {
 protected:
  const DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
  UINT width = 0;
  UINT height = 0;
  UINT depth = 1;

  const Descriptor* srv = nullptr;
  DescriptorHeap* srvHeap = nullptr;
  CommandQueue* cmdQueue = nullptr;

  virtual void allocateResource();
  virtual void allocateDescriptor();

  void* getData(const char* filePath, UINT* width, UINT* height);

 public:
  DXGI_FORMAT getFormat() const { return format; }
  UINT getWidth() const { return width; }
  UINT getHeight() const { return height; }
  UINT getDepth() const { return depth; }
  const Descriptor& getSrv() const { return *srv; }

  virtual void clear(CommandList* cmdList, float* clearValue = nullptr) {
    assert(false);
  }
  void resize(UINT newWidth, UINT newHeight, UINT newDepth = 1);
  void loadData(UINT dataWidth, UINT dataHeight, void* data);
  void loadData(UINT dataWidth, UINT dataHeight,
                std::vector<std::string> filePath);

  explicit Texture(DescriptorHeap* dec, CommandQueue* queue, DXGI_FORMAT format)
      : format(format), srvHeap(dec), cmdQueue(queue) {}
  explicit Texture(DescriptorHeap* dec, CommandQueue* queue, DXGI_FORMAT format,
                   UINT width, UINT height, UINT depth = 1);
  explicit Texture(DescriptorHeap* dec, CommandQueue* queue, DXGI_FORMAT format,
                   UINT width, UINT height, void* data);
  explicit Texture(DescriptorHeap* dec, CommandQueue* queue, DXGI_FORMAT format,
                   const char* filePath);
  explicit Texture(DescriptorHeap* dec, CommandQueue* queue, DXGI_FORMAT format,
                   UINT width, UINT height, std::vector<std::string> filePath);
  explicit Texture(DescriptorHeap* dec, CommandQueue* queue, DXGI_FORMAT format,
                   UINT width, UINT height, std::vector<float*> dataList);
};

class RootParameter {
 protected:
  D3D12_ROOT_PARAMETER param = {};

  friend class RootSignature;
  friend class RaytracingRootSignature;

  virtual ~RootParameter() {}

 public:
  virtual bool isRootTable() const { return false; }
  virtual void bindCompute(ID3D12GraphicsCommandList* cmdList,
                           UINT paramIdx) const = 0;
  virtual void bindGraphics(ID3D12GraphicsCommandList* cmdList,
                            UINT paramIdx) const = 0;
};

class RootTable : public RootParameter {
  D3D12_GPU_DESCRIPTOR_HANDLE tableStart;
  std::vector<D3D12_DESCRIPTOR_RANGE> ranges = {};

 public:
  RootTable() = delete;
  explicit RootTable(const char* code) { create(code); }
  explicit RootTable(const char* code, D3D12_GPU_DESCRIPTOR_HANDLE handle) {
    create(code);
    setTableStart(handle);
  }
  void operator=(D3D12_GPU_DESCRIPTOR_HANDLE handle) { setTableStart(handle); }

  void setTableStart(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
    tableStart = handle;
  }

  void create(const char* code);
  bool isRootTable() const override { return true; }
  void bindCompute(ID3D12GraphicsCommandList* cmdList,
                   UINT paramIdx) const override;
  void bindGraphics(ID3D12GraphicsCommandList* cmdList,
                    UINT paramIdx) const override;
};

class RootPointer : public RootParameter {
  D3D12_GPU_VIRTUAL_ADDRESS resourceAddress;

 public:
  RootPointer() = delete;
  explicit RootPointer(const char* code) { create(code); }
  explicit RootPointer(const char* code, D3D12_GPU_VIRTUAL_ADDRESS address) {
    create(code);
    setResourceAddress(address);
  }
  void operator=(D3D12_GPU_VIRTUAL_ADDRESS address) {
    setResourceAddress(address);
  }

  void setResourceAddress(D3D12_GPU_VIRTUAL_ADDRESS address) {
    resourceAddress = address;
  }

  void create(const char* code);
  void bindCompute(ID3D12GraphicsCommandList* cmdList,
                   UINT paramIdx) const override;
  void bindGraphics(ID3D12GraphicsCommandList* cmdList,
                    UINT paramIdx) const override;
};

class RootConstants : public RootParameter {
  std::vector<UINT> _32BitConsts;
  std::type_index typeId = typeid(void);

 public:
  RootConstants() = delete;
  RootConstants(RootConstants&& rc)
      : RootParameter(rc),
        typeId(rc.typeId),
        _32BitConsts(std::move(rc._32BitConsts)) {}
  template <typename T>
  explicit RootConstants(const char* code, const T& constants) {
    _32BitConsts.resize((sizeof(T) + 3) / 4);
    typeId = std::type_index(typeid(T));
    create(code);
    setConstants(constants);
  }
  template <typename T>
  void operator=(const T& constants) {
    setConstants(constants);
  }
  template <typename T>
  void setConstants(const T& constants) {
    assert(std::type_index(typeid(T)) == typeId);
    memcpy(_32BitConsts.data(), &constants, sizeof(T));
  }

  void create(const char* code);
  void bindCompute(ID3D12GraphicsCommandList* cmdList,
                   UINT paramIdx) const override;
  void bindGraphics(ID3D12GraphicsCommandList* cmdList,
                    UINT paramIdx) const override;
};

class RootSignature {
 public:
  //std::variant : it is a container that stores one of several types
  using VarRootParam = std::variant<RootConstants, RootPointer, RootTable>;

 private:
  ID3D12RootSignature* rootSig = nullptr;
  bool bIncludeRootTable = false;
  std::vector<VarRootParam> params;
  std::map<std::string_view, UINT> indices;

 public:
  ~RootSignature();
  RootSignature() {}
  ID3D12RootSignature* get() const { return rootSig; }
  bool includeRootTable() const { return bIncludeRootTable; }

  void build(D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
  void build(const std::vector<D3D12_STATIC_SAMPLER_DESC>& samplerArr,
             D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
  void bindCompute(ID3D12GraphicsCommandList* cmdList,
                   DescriptorHeap* heap) const;
  void bindGraphics(ID3D12GraphicsCommandList* cmdList,
                    DescriptorHeap* heap) const;

  RootSignature(
      std::initializer_list<std::pair<std::string_view, VarRootParam>> list) {
    for (auto& [str, var] : list) {
      params.push_back(std::move(*const_cast<VarRootParam*>(&var)));
      indices.emplace(str, params.size() - 1);
    }
  }

  template <typename T>
  void set(std::string_view name, const T& data) {
    std::any any_data = data;

    if (params[indices[name]].index() == 0) {
      std::get<RootConstants>(params[indices[name]]).setConstants(data);
    } else if (params[indices[name]].index() == 1) {
      std::get<RootPointer>(params[indices[name]])
          .setResourceAddress(
              std::any_cast<D3D12_GPU_VIRTUAL_ADDRESS>(any_data));
    } else {
      std::get<RootTable>(params[indices[name]])
          .setTableStart(std::any_cast<Descriptor>(any_data).getGpuHandle());
    }
  }
};

class RenderTarget : public Texture {
  const Descriptor* rtv = nullptr;
  DescriptorHeap* rtvHeap = nullptr;

  void allocateResource() override;
  void allocateDescriptor() override;

 public:
  const Descriptor& getRtv() const { return *rtv; }
  void clear(CommandList* cmdList, float* clearValue = nullptr) override;

  RenderTarget(DescriptorHeap* srvHeap, DescriptorHeap* _rtvHeap,
               CommandQueue* queue, DXGI_FORMAT format)
      : Texture(srvHeap, queue, format), rtvHeap(_rtvHeap) {}
  RenderTarget(DescriptorHeap* srvHeap, DescriptorHeap* _rtvHeap,
               CommandQueue* queue, DXGI_FORMAT format, UINT width,
               UINT height);
};

class DepthTarget : public Texture {
  const Descriptor* dsv = nullptr;
  DescriptorHeap* dsvHeap = nullptr;
  void allocateResource() override;
  void allocateDescriptor() override;

 public:
  const Descriptor& getDsv() const { return *dsv; }
  void clear(CommandList* cmdList, float* clearValue = nullptr) override;

  DepthTarget(DescriptorHeap* dec, DescriptorHeap* _dsvHeap,
              CommandQueue* queue, DXGI_FORMAT format)
      : Texture(dec, queue, format), dsvHeap(_dsvHeap) {}
  DepthTarget(DescriptorHeap* dec, DescriptorHeap* _dsvHeap,
              CommandQueue* queue, DXGI_FORMAT format, UINT width, UINT height);
};

class ComputeTarget : public Texture {
  const Descriptor* uav = nullptr;
  DescriptorHeap* uavHeap = nullptr;
  void allocateResource() override;
  void allocateDescriptor() override;

 public:
  const Descriptor& getUav() const { return *uav; }
  void clear(CommandList* cmdList, float* clearValue = nullptr) override;

  ComputeTarget(DescriptorHeap* dec, DescriptorHeap* _uavHeap,
                CommandQueue* queue, DXGI_FORMAT format)
      : Texture(dec, queue, format), uavHeap(_uavHeap) {}
  ComputeTarget(DescriptorHeap* dec, DescriptorHeap* _uavHeap,
                CommandQueue* queue, DXGI_FORMAT format, UINT width,
                UINT height);
};

class ReadbackBuffer {
 protected:
  ID3D12Resource* readbackBuffer = nullptr;
  void* cpuAddress = nullptr;
  UINT64 maxReadbackSize{};

 public:
  ~ReadbackBuffer() { destroy(); }
  ReadbackBuffer() {}
  explicit ReadbackBuffer(UINT64 requiredSize) { create(requiredSize); }

  ID3D12Resource* get() { return readbackBuffer; }
  void destroy();
  void create(UINT64 requiredSize);
  void* map();
  void unmap();
  void readback(ID3D12GraphicsCommandList* cmdList, const dxResource& source);
};

class dxShader {
  ID3DBlob* code{};

 public:
  ~dxShader() { flush(); }
  dxShader() {}
  dxShader(const char* hlslFile, const char* entryFtn, const char* target) {
    load(hlslFile, entryFtn, target);
  }

  D3D12_SHADER_BYTECODE getCode() const {
    D3D12_SHADER_BYTECODE shadercode;
    shadercode.pShaderBytecode = code->GetBufferPointer();
    shadercode.BytecodeLength = code->GetBufferSize();
    return shadercode;
  }

  void flush() { SAFE_RELEASE(code); }
  void load(const char* hlslFile, const char* entryFtn, const char* target);
};

class GraphicsPipeline {
  static const LONG maxTargetSize = 16000;

  // set in build()
  ID3D12PipelineState* pipeline = nullptr;
  const RootSignature* pRootSig = nullptr;
  UINT numRts = 0;
  bool enableDepth = false;

  // sized in & set after build()
  std::vector<const Descriptor*> dscrHandles;
  DescriptorHeap* decHeap = nullptr;

  // set after build()
  D3D12_VIEWPORT viewport{};
  D3D12_RECT scissorRect = {0, 0, maxTargetSize, maxTargetSize};

  mutable ID3D12GraphicsCommandList* currentCmdList = nullptr;
  mutable std::vector<D3D12_RESOURCE_BARRIER> beginBarriers;
  mutable std::vector<D3D12_RESOURCE_BARRIER> endBarriers;
  mutable bool doClear = false;

 public:
  explicit GraphicsPipeline(DescriptorHeap* heap) : decHeap(heap) {}
  ~GraphicsPipeline() { destroy(); }
  void destroy();

  void build(const RootSignature* rootSig, D3D12_INPUT_LAYOUT_DESC inputLayout,
             D3D12_SHADER_BYTECODE vsCode, D3D12_SHADER_BYTECODE psCode,
             UINT numRts, const std::vector<BlendMode>& blendModes,
             DepthMode depthMode, const std::vector<DXGI_FORMAT>& renderFormats,
             DXGI_FORMAT depthFormat, D3D12_CULL_MODE cullMode);

  void setRtHandle(UINT rtIdx, const Descriptor& rtv) {
    assert(rtIdx < numRts);
    dscrHandles[rtIdx] = &rtv;
  }

  void setDtHandle(const Descriptor& rtv) {
    assert(enableDepth);
    dscrHandles[numRts] = &rtv;
  }

  void setViewport(const D3D12_VIEWPORT& viewport) {
    this->viewport = viewport;
  }
  void setScissorRect(const D3D12_RECT& rect) { scissorRect = rect; }
  void clearTargetsBeforeNextRender() const { doClear = true; }
  void begin(ID3D12GraphicsCommandList* cmdList) const;
  void end() const;

  void SetName(const wchar_t* name) {
    pipeline->SetName((std::wstring(L"[PSO: ") + name + L"]").c_str());
  }
};

class SwapChain {
  struct IDXGISwapChain3* swapChain = nullptr;
  UINT numBuffers = 0;
  DXGI_FORMAT bufferFormat{};
  UINT bufferW{};
  UINT bufferH{};
  UINT flags{};
  UINT currentBufferIdx{};
  Descriptor currentRtv{};

  struct BackBuffer : dxResource {
    const Descriptor* rtv = nullptr;
    BackBuffer() {}
    BackBuffer(ID3D12Resource* resource);
    BackBuffer(ID3D12Resource* resource, DescriptorHeap* rtvHeap);
  };
  std::vector<BackBuffer> backBuffers;

 public:
  UINT getBufferCount() const { return numBuffers; }
  DXGI_FORMAT getBufferFormat() const { return bufferFormat; }
  const Descriptor& getRtv() const { return currentRtv; }

  ~SwapChain();
  void present();
  void resize(UINT width, UINT height);
  void create(HWND hwnd, DescriptorHeap* rtvHeap, CommandQueue* cmdQueue,
              UINT width, UINT height, UINT numBackBuffers,
              DXGI_FORMAT bufferFormat, bool allowTearing = false);
};

/*-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --*/


class MeshData {
 public:
  DxBuffer vtxBuff = DxBuffer(DxBuffer::StorageType::gpu);
  DxBuffer idxBuff = DxBuffer(DxBuffer::StorageType::gpu);
  DxBuffer wireIdxBuffer = DxBuffer(DxBuffer::StorageType::gpu);
  XMMATRIX modelMat = XMMatrixIdentity();
  ID3D12Resource* blas = nullptr;
  ID3D12Resource* tlas = nullptr;

  explicit MeshData(CommandQueue* cmdQueue, CommandList* cmdList,
                    const char* filePath, UINT flipX = false,
                    UINT flipY = false, UINT flipZ = false,
                    bool flipTexV = false, bool centering = true,
                    bool buildAS = false, bool needWire = false);
  MeshData() {}
  ~MeshData() {
    SAFE_RELEASE(blas);
    SAFE_RELEASE(tlas);
  }

  struct RenderInfo {
    D3D12_VERTEX_BUFFER_VIEW vtxBuffView{};
    D3D12_INDEX_BUFFER_VIEW idxBuffView{};
    UINT numTriangles{};
    D3D12_INDEX_BUFFER_VIEW wireIdxBuffView{};
    UINT numWire{};
  } renderInfo;

  struct WireIndexDuplicate {
    UINT x;
    UINT y;

    bool operator<(const WireIndexDuplicate& target) const {
      const UINT *lx, *ly, *rx, *ry;
      if (x < y) {
        lx = &x;
        ly = &y;
      } else {
        lx = &y;
        ly = &x;
      }
      if (target.x < target.y) {
        rx = &target.x;
        ry = &target.y;
      } else {
        rx = &target.y;
        ry = &target.x;
      }

      return (*lx != *rx) ? (*lx < *rx) : (*ly < *ry);
    }
  };

  const RenderInfo& getRenderInfo() const { return renderInfo; }

  static D3D12_INPUT_LAYOUT_DESC getInputLayout() {
    static D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    return {inputElementDescs, _countof(inputElementDescs)};
  }
};
