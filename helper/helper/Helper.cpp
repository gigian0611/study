#include "Helper.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

struct Vertex {
  float3 position;
  float3 normal;
  float2 texcoord;
};

class compTynyIdx {
 public:
  bool operator()(const tinyobj::index_t& a, const tinyobj::index_t& b) const {
    return (a.vertex_index != b.vertex_index)
               ? (a.vertex_index < b.vertex_index)
           : (a.normal_index != b.normal_index)
               ? (a.normal_index < b.normal_index)
               : (a.texcoord_index < b.texcoord_index);
  }
};

void loadOBJFile(const char* filename, std::vector<float>* vertices,
                 std::vector<UINT>* indices, bool* writeNormal,
                 bool* writeTexcoord) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  bool ret =
      tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename);

  if (!err.empty()) {
    Error(err.c_str());
  }
  if (shapes.size() != 1) {
    Error("The obj file includes several meshes.\n");
  }
  std::vector<tinyobj::index_t>& I = shapes[0].mesh.indices;
  UINT numTri = static_cast<UINT>(shapes[0].mesh.num_face_vertices.size());
  if (I.size() != static_cast<UINT64>(3) * numTri) {
    Error("The mesh includes non-triangle faces.\n");
  }
  (*writeNormal) = I[0].normal_index != -1 ? true : false;
  (*writeTexcoord) = I[0].texcoord_index != -1 ? true : false;

  std::map<tinyobj::index_t, UINT, compTynyIdx> tinyIdxToVtxIdx;
  UINT numVtx = 0;

  vertices->resize(numTri * 3 * 8);
  indices->resize(numTri * 3);
  Vertex* pV = reinterpret_cast<Vertex*>(vertices->data());

  for (UINT i = 0; i < 3 * numTri; ++i) {
    tinyobj::index_t& tinyIdx = I[i];
    auto iterAndBool = tinyIdxToVtxIdx.insert({tinyIdx, numVtx});

    if (iterAndBool.second) {
      pV[numVtx].position =
          *((float3*)&attrib
                .vertices[static_cast<size_t>(tinyIdx.vertex_index * 3)]);
      if (writeNormal)
        pV[numVtx].normal =
            *((float3*)&attrib
                  .normals[static_cast<size_t>(tinyIdx.normal_index * 3)]);
      if (writeTexcoord)
        pV[numVtx].texcoord =
            *((float2*)&attrib
                  .texcoords[static_cast<size_t>(tinyIdx.texcoord_index * 2)]);
      numVtx++;
    }
    (*indices)[i] = iterAndBool.first->second;
  }
  vertices->resize(numVtx * 8);
}

void readRegisterInfo(const char* token, DescriptorType* type,
                      UINT* registerSpace, UINT* registerBase,
                      UINT* numDescriptors) {
  UINT i = 0;

  if (token[0] == '(') {
    ++i;
    i += readDigit(token + i, registerSpace);
    if (token[i] != ')') Error("fail to register a root parameter");
    ++i;
  } else {
    *registerSpace = 0;
  }

  UINT typePos = i;

  if (token[i] == 't') {
    *type = DescriptorType::SRV;
  } else if (token[i] == 'u') {
    *type = DescriptorType::UAV;
  } else if (token[i] == 'b') {
    *type = DescriptorType::CBV;
  } else {
    Error("fail to register a root parameter");
  }

  ++i;

  i += readDigit(token + i, registerBase);

  if (token[i] == '\0') {
    *numDescriptors = 1;
    return;
  } else if (token[i] != '-') {
    Error("fail to register a root parameter");
  }

  ++i;

  if (token[i] == '\0') {
    *numDescriptors = UINT(-1);  // unbounded number of descriptors
    return;
  } else if (token[i] != token[typePos]) {
    Error("fail to register a root parameter");
  }

  ++i;

  UINT registerEnd;
  i += readDigit(token + i, &registerEnd);

  *numDescriptors = registerEnd - *registerBase + 1;

  if (token[i] != '\0') Error("fail to register a root parameter");
}

 template <typename T>
 T* _loadImage(const char* filepath, UINT& width, UINT& height,
               UINT& readChannels, UINT writeChannels) {
   assert(false);
 }

 template <>
 UINT8* _loadImage(const char* filepath, UINT& width, UINT& height,
                   UINT& readChannels, UINT writeChannels) {
   return stbi_load(filepath, (int*)&width, (int*)&height, (int*)&readChannels,
                    (int)writeChannels);
 }

 template <>
 float* _loadImage(const char* filepath, UINT& width, UINT& height,
                   UINT& readChannels, UINT writeChannels) {
   return stbi_loadf(filepath, (int*)&width, (int*)&height,
   (int*)&readChannels,
                     (int)writeChannels);
 }

 template <typename T>
 T* loadImage(const char* filepath, UINT& width, UINT& height,
              UINT writeChannels) {
   UINT readChannels;
   T* image =
       _loadImage<T>(filepath, width, height, readChannels, writeChannels);

   if (!image) {
     Error(stbi_failure_reason());
   }

   return image;
 }

 UINT8* loadImage_uint8(const char* filePath, UINT& width, UINT& height,
                         UINT reqChannels) {
   return loadImage<UINT8>(filePath, width, height, reqChannels);
 }

 float* loadImage_float(const char* filePath, UINT& width, UINT& height,
                         UINT reqChannels) {
   return loadImage<float>(filePath, width, height, reqChannels);
 }

 void clearTargets(CommandQueue& cmdqueue, CommandList& cmdList,
                   const std::vector<const class Descriptor*>& rtvs,
                   const std::vector<const class Descriptor*>& dsvs) {
   auto* rawList = cmdList.begin();
   {
     float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
     std::vector<D3D12_RESOURCE_BARRIER> barriers;
     std::vector<D3D12_RESOURCE_BARRIER> endBarriers;
     barriers.reserve(rtvs.size() + dsvs.size());
     endBarriers.reserve(barriers.size());

     for (const Descriptor* dscr : rtvs) {
      const dxResource& rsc = *dscr->getResource();
      if (rsc.getState() != D3D12_RESOURCE_STATE_RENDER_TARGET) {
        barriers.push_back(Transition(rsc.get(), rsc.getState(),
                                      D3D12_RESOURCE_STATE_RENDER_TARGET));
        endBarriers.push_back(Transition(
            rsc.get(), D3D12_RESOURCE_STATE_RENDER_TARGET, rsc.getState()));
      }
     }
     for (const Descriptor* dscr : dsvs) {
      const dxResource& rsc = *dscr->getResource();
      if (rsc.getState() != D3D12_RESOURCE_STATE_DEPTH_WRITE) {
        barriers.push_back(Transition(rsc.get(), rsc.getState(),
                                      D3D12_RESOURCE_STATE_DEPTH_WRITE));
        endBarriers.push_back(Transition(
            rsc.get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, rsc.getState()));
      }
     }

     rawList->ResourceBarrier(barriers.size(), barriers.data());
     for (const Descriptor* dscr : rtvs) {
      rawList->ClearRenderTargetView(dscr->getCpuHandle(), black, 0, nullptr);
     }
     for (const Descriptor* dscr : dsvs) {
      rawList->ClearDepthStencilView(
          dscr->getCpuHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0x00, 0, nullptr);
     }
     rawList->ResourceBarrier(endBarriers.size(), endBarriers.data());
   }
   cmdList.end(&cmdqueue);
 }

CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type) {
  this->type = type;
  D3D12_COMMAND_QUEUE_DESC cqDesc = {
      type,
  };
  ThrowFailedHR(
      getDevice()->get()->CreateCommandQueue(
      &cqDesc, IID_PPV_ARGS(&cmdQueue)));
  ThrowFailedHR(getDevice()->get()->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                                      IID_PPV_ARGS(&fence)));
}

UINT64 CommandQueue::excuteCommandList(ID3D12CommandList* rawList) {
  cmdQueue->ExecuteCommandLists(1, &rawList);
  ++fenceValue;
  cmdQueue->Signal(fence, fenceValue); // Signal : Function to be notified when the fence value reaches the desired value
  return fenceValue;
}

CommandQueue::~CommandQueue() {
  SAFE_RELEASE(cmdQueue);
  SAFE_RELEASE(fence);
}

void CommandQueue::waitCompletion(UINT64 targetValue, HANDLE handle) {
  //fence->GetCompletedValue() // return fence value
  ThrowFailedHR(fence->SetEventOnCompletion( // Call a handle event when the fence value reaches a specific value
      targetValue ? targetValue : fenceValue, handle)); // If the handle is null, it is not returned until conditional
}

void CommandQueue::signalForSafety() { cmdQueue->Signal(fence, 1000); }

CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type) {
  this->type = type;
  ThrowFailedHR(getDevice()->get()->CreateCommandAllocator(
      type, IID_PPV_ARGS(&cmdAllocator)));
  ThrowFailedHR(getDevice()->get()->CreateCommandList(
      0, type, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList)));
  ThrowFailedHR(cmdList->Close());
}

void CommandList::reset() { ThrowFailedHR(cmdAllocator->Reset()); }

ID3D12GraphicsCommandList* CommandList::begin() {
  ThrowFailedHR(cmdList->Reset(cmdAllocator, nullptr));
  return cmdList;
}

UINT64 CommandList::end(CommandQueue* queue, bool waitFinish) {
  ThrowFailedHR(cmdList->Close());

  UINT64 fenceValue = queue->excuteCommandList(cmdList);

  if (waitFinish) queue->waitCompletion(fenceValue);

  return fenceValue;
}

CommandList::~CommandList() {
  SAFE_RELEASE(cmdAllocator);
  SAFE_RELEASE(cmdList);
}

D3D12_RESOURCE_STATES dxResource::changeResourceState(
    ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState) const {
  if (resourceState == newState) return resourceState;

  D3D12_RESOURCE_STATES prevState = resourceState;
  resourceState = newState;
  auto tras = Transition(resource, prevState, newState);
  cmdList->ResourceBarrier(1, &tras);
  return prevState;
}

void DxBuffer::create(UINT64 bufferSize, D3D12_HEAP_TYPE heapType,
                      D3D12_RESOURCE_FLAGS rscFlag,
                      D3D12_RESOURCE_STATES rscState) {
  if (resource != nullptr)
    Error("destroy() must be called before re-creating.");
  resourceState = rscState;
  resource = createCommittedBuffer(bufferSize, heapType, rscFlag, rscState);
  gpuAddress = resource->GetGPUVirtualAddress();
  this->bufferSize = bufferSize;
}

void* DxBuffer::map() {
  if (!cpuAddress) {
    auto range = Range(0);
    assert(getBufferSize() > 0);
    if (type == StorageType::cpu) {
      resource->Map(0, &range, &cpuAddress);
    } else {
      assert(!uploader);
      uploader = createCommittedBuffer(getBufferSize(), D3D12_HEAP_TYPE_UPLOAD,
                                       D3D12_RESOURCE_FLAG_NONE,
                                       D3D12_RESOURCE_STATE_GENERIC_READ);
      uploader->Map(0, &range, &cpuAddress);
    }
  }

  return cpuAddress;
}

void DxBuffer::unmap(CommandQueue* cmdQueue, CommandList* cmdList) {
  assert(cpuAddress);
  if (type == StorageType::cpu) {
    resource->Unmap(0, nullptr);
  } else {
    uploader->Unmap(0, nullptr);

    auto* rawList = cmdList->begin();
    {
      D3D12_RESOURCE_STATES prevState =
          changeResourceState(rawList, D3D12_RESOURCE_STATE_COPY_DEST);
      rawList->CopyBufferRegion(resource, 0, uploader, 0, getBufferSize());
      changeResourceState(rawList, prevState);
    }
    UINT64 fenceValue = cmdList->end(cmdQueue);

    cmdQueue->waitCompletion(fenceValue);
    uploader->Release();
    uploader = nullptr;
  }
  cpuAddress = nullptr;
}

ID3D12Resource* createCommittedBuffer(UINT64 bufferSize,
                                      D3D12_HEAP_TYPE heapType,
                                      D3D12_RESOURCE_FLAGS resourceFlags,
                                      D3D12_RESOURCE_STATES resourceStates) {
  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  desc.Format = DXGI_FORMAT_UNKNOWN;
  desc.Width = bufferSize;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  desc.Flags = resourceFlags;

  D3D12_HEAP_PROPERTIES prop = {};
  prop.Type = heapType;

  ID3D12Resource* resource;
  ThrowFailedHR(getDevice()->get()->CreateCommittedResource(
      &prop, D3D12_HEAP_FLAG_NONE, &desc, resourceStates, nullptr,
      IID_PPV_ARGS(&resource)));

  static UINT i = 0;
  resource->SetName((std::wstring(L"buffer") + std::to_wstring(i++)).c_str());

  return resource;
}

DescriptorHeap::DescriptorHeap(UINT maxDescriptors,
                               D3D12_DESCRIPTOR_HEAP_TYPE heapType)
    : type(heapType) {
  if (heap != nullptr) Error("destroy() must be called before re-creating.");

  D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
  {
    heapDesc.Type = type;
    heapDesc.NumDescriptors = maxDescriptors > 0 ? maxDescriptors : 1;
    heapDesc.Flags = (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
                         ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                         : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  }
  getDevice()->get()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

  D3D12_CPU_DESCRIPTOR_HANDLE cpuAddr =
      heap->GetCPUDescriptorHandleForHeapStart();
  D3D12_GPU_DESCRIPTOR_HANDLE gpuAddr =
      heap->GetGPUDescriptorHandleForHeapStart();
  UINT descriptorSize =
      getDevice()->get()->GetDescriptorHandleIncrementSize(type);

  descriptorArr.reserve(maxDescriptors);

  for (UINT i = 0; i < maxDescriptors; ++i) {
    descriptorArr.push_back(Descriptor(cpuAddr, gpuAddr));
    cpuAddr.ptr = cpuAddr.ptr + descriptorSize;
    gpuAddr.ptr = gpuAddr.ptr + descriptorSize;
  }
}

DescriptorHeap::~DescriptorHeap() {
  SAFE_RELEASE(heap);
  descriptorArr.clear();
}

void DescriptorHeap::bind(ID3D12GraphicsCommandList* cmdList) const {
  cmdList->SetDescriptorHeaps(1, &heap);
}

void Texture::allocateResource() {
  D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE;
  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  resource = createCommittedTexture(format, width, height, depth, flag, state,
                                    nullptr);
  resourceState = state;
}

ID3D12Resource* createCommittedTexture(DXGI_FORMAT format, UINT width,
                                       UINT height, UINT depth,
                                       D3D12_RESOURCE_FLAGS resourceFlags,
                                       D3D12_RESOURCE_STATES resourceStates,
                                       D3D12_CLEAR_VALUE* pOptClearValue) {
  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  desc.Format = format;
  desc.Width = width;
  desc.Height = height;
  desc.DepthOrArraySize = depth;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Flags = resourceFlags;

  D3D12_HEAP_PROPERTIES prop = {};
  prop.Type = D3D12_HEAP_TYPE_DEFAULT;

  ID3D12Resource* resource;
  ThrowFailedHR(getDevice()->get()->CreateCommittedResource(
      &prop, D3D12_HEAP_FLAG_NONE, &desc, resourceStates, pOptClearValue,
      IID_PPV_ARGS(&resource)));

  static UINT i = 0;
  resource->SetName((std::wstring(L"texture") + std::to_wstring(i++)).c_str());

  return resource;
}

void Texture::allocateDescriptor() {
  if (srv) {
    srv->assignSrv(*this);
  } else {
    srv = srvHeap->assignSrv(*this);
  }
}

void* Texture::getData(const char* filePath, UINT* width, UINT* height) {
  UINT channel = 4;
  switch (format) {
    case DXGI_FORMAT_R16G16B16A16_UNORM:
      //return loadUint16Data(filePath, width, height);
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
      return loadImage_float(filePath, *width, *height, channel);
    case DXGI_FORMAT_R8G8B8A8_UNORM:
      return loadImage_uint8(filePath, *width, *height, channel);
    default:
      break;
  }
  return nullptr;
}

void Texture::resize(UINT newWidth, UINT newHeight, UINT newDepth) {
  SAFE_RELEASE(resource);
  width = newWidth;
  height = newHeight;
  depth = newDepth;
  allocateResource();
  allocateDescriptor();
}

void Texture::loadData(UINT dataWidth, UINT dataHeight, void* data) {
  if (width < dataWidth || height < dataHeight) {
    resize(dataWidth, dataHeight);
  }

  UINT dataRowPitch = _bpp(format) * dataWidth;
  UINT textureRowPitch =
      _align(_bpp(format) * width, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
  UINT enoughSize = textureRowPitch * height;

  ID3D12Resource* uploader = createCommittedBuffer(
      enoughSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE,
      D3D12_RESOURCE_STATE_GENERIC_READ);
  void* cpuAddress;
  auto range = Range(0);
  uploader->Map(0, &range, &cpuAddress);

  UINT8* dst = reinterpret_cast<UINT8*>(cpuAddress);
  UINT8* src = reinterpret_cast<UINT8*>(data);
  if (dataRowPitch == textureRowPitch) {
    memcpy(dst, src, (UINT64)textureRowPitch * height);
  } else {
    for (UINT i = 0; i < height; ++i) {
      memcpy(dst, src, dataRowPitch);
      src += dataRowPitch;
      dst += textureRowPitch;
    }
  }

  D3D12_TEXTURE_COPY_LOCATION dstDesc;
  dstDesc.pResource = resource;
  dstDesc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  dstDesc.SubresourceIndex = 0;

  D3D12_TEXTURE_COPY_LOCATION srcDesc = {};
  srcDesc.pResource = uploader;
  srcDesc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  srcDesc.PlacedFootprint.Footprint.Depth = 1;
  srcDesc.PlacedFootprint.Footprint.Format = format;
  srcDesc.PlacedFootprint.Footprint.Width = dataWidth;
  srcDesc.PlacedFootprint.Footprint.Height = dataHeight;
  srcDesc.PlacedFootprint.Footprint.RowPitch = textureRowPitch;

  CommandList* cmdList = new CommandList;
  auto* rawList = cmdList->begin();
  {
    D3D12_RESOURCE_STATES prevState =
        changeResourceState(rawList, D3D12_RESOURCE_STATE_COPY_DEST);
    rawList->CopyTextureRegion(&dstDesc, 0, 0, 0, &srcDesc, nullptr);
    changeResourceState(rawList, prevState);
  }
  UINT64 fenceValue = cmdList->end(cmdQueue);

  cmdQueue->waitCompletion(fenceValue);
  uploader->Release();
  cmdList->reset();
  delete cmdList;
  // delete data;
}

void Texture::loadData(UINT dataWidth, UINT dataHeight,
                       std::vector<std::string> filePath) {
  resize(dataWidth, dataHeight, UINT(filePath.size()));

  UINT length = UINT(filePath.size());
  UINT dataRowPitch = _bpp(format) * width;
  UINT textureRowPitch =
      _align(_bpp(format) * width, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
  UINT enoughSize = textureRowPitch * height;
  std::vector<ID3D12Resource*> uploader;

  for (UINT i = 0; i < length; i++) {
    void* data = getData(filePath[i].c_str(), &dataWidth, &dataHeight);
    uploader.push_back(createCommittedBuffer(
        enoughSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATE_GENERIC_READ));
    void* cpuAddress;
    auto range = Range(0);
    uploader[i]->Map(0, &range, &cpuAddress);

    UINT* dst = reinterpret_cast<UINT*>(cpuAddress);
    UINT* src = reinterpret_cast<UINT*>(data);
    if (dataRowPitch == textureRowPitch) {
      memcpy(dst, src, (UINT64)textureRowPitch * height);
    } else {
      for (UINT i = 0; i < height; ++i) {
        memcpy(dst, src, dataRowPitch);
        src += dataRowPitch;
        dst += textureRowPitch;
      }
    }

    D3D12_TEXTURE_COPY_LOCATION dstDesc;
    dstDesc.pResource = resource;
    dstDesc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstDesc.SubresourceIndex = i;

    D3D12_TEXTURE_COPY_LOCATION srcDesc = {};
    srcDesc.pResource = uploader[i];
    srcDesc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcDesc.PlacedFootprint.Footprint.Depth = 1;
    srcDesc.PlacedFootprint.Footprint.Format = format;
    srcDesc.PlacedFootprint.Footprint.Width = width;
    srcDesc.PlacedFootprint.Footprint.Height = height;
    srcDesc.PlacedFootprint.Footprint.RowPitch = textureRowPitch;

    CommandList* cmdList = new CommandList;
    auto* rawList = cmdList->begin();
    {
      D3D12_RESOURCE_STATES prevState =
          changeResourceState(rawList, D3D12_RESOURCE_STATE_COPY_DEST);
      rawList->CopyTextureRegion(&dstDesc, 0, 0, 0, &srcDesc, nullptr);
      changeResourceState(rawList, prevState);
    }
    UINT64 fenceValue = cmdList->end(cmdQueue);

    cmdQueue->waitCompletion(fenceValue);
    uploader[i]->Release();
    cmdList->reset();
    delete cmdList;
    delete[] data;
  }
}

Texture::Texture(DescriptorHeap* dec, CommandQueue* queue, DXGI_FORMAT format,
                 UINT width, UINT height, UINT depth)
    : format(format), srvHeap(dec), cmdQueue(queue) {
  resize(width, height, depth);
}

Texture::Texture(DescriptorHeap* dec, CommandQueue* queue, DXGI_FORMAT format,
                 UINT width, UINT height, void* data)
    : format(format), srvHeap(dec), cmdQueue(queue) {
  resize(width, height, 1);
  loadData(width, height, data);
}

Texture::Texture(DescriptorHeap* dec, CommandQueue* queue, DXGI_FORMAT format,
                 const char* filePath)
    : format(format), srvHeap(dec), cmdQueue(queue) {
  void* data = getData(filePath, &width, &height);
  resize(width, height, 1);
  loadData(width, height, data);
}

Texture::Texture(DescriptorHeap* dec, CommandQueue* queue, DXGI_FORMAT format,
                 UINT width, UINT height, std::vector<std::string> filePath)
    : format(format), srvHeap(dec), cmdQueue(queue) {
  loadData(width, height, filePath);
}

Texture::Texture(DescriptorHeap* dec, CommandQueue* queue, DXGI_FORMAT format,
                 UINT width, UINT height, std::vector<float*> dataList)
    : format(format), srvHeap(dec), cmdQueue(queue) {
  resize(width, height, UINT(dataList.size()));

  UINT length = UINT(dataList.size());
  UINT dataRowPitch = _bpp(format) * width;
  UINT textureRowPitch =
      _align(_bpp(format) * width, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
  UINT enoughSize = textureRowPitch * height;
  std::vector<ID3D12Resource*> uploader;

  for (UINT i = 0; i < length; i++) {
    void* data = dataList[i];
    uploader.push_back(createCommittedBuffer(
        enoughSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATE_GENERIC_READ));
    void* cpuAddress;
    auto range = Range(0);
    uploader[i]->Map(0, &range, &cpuAddress);

    UINT* dst = reinterpret_cast<UINT*>(cpuAddress);
    UINT* src = reinterpret_cast<UINT*>(data);
    if (dataRowPitch == textureRowPitch) {
      memcpy(dst, src, (UINT64)textureRowPitch * height);
    } else {
      for (UINT i = 0; i < height; ++i) {
        memcpy(dst, src, dataRowPitch);
        src += dataRowPitch;
        dst += textureRowPitch;
      }
    }

    D3D12_TEXTURE_COPY_LOCATION dstDesc;
    dstDesc.pResource = resource;
    dstDesc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstDesc.SubresourceIndex = i;

    D3D12_TEXTURE_COPY_LOCATION srcDesc = {};
    srcDesc.pResource = uploader[i];
    srcDesc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcDesc.PlacedFootprint.Footprint.Depth = 1;
    srcDesc.PlacedFootprint.Footprint.Format = format;
    srcDesc.PlacedFootprint.Footprint.Width = width;
    srcDesc.PlacedFootprint.Footprint.Height = height;
    srcDesc.PlacedFootprint.Footprint.RowPitch = textureRowPitch;

    CommandList* cmdList = new CommandList;
    auto* rawList = cmdList->begin();
    {
      D3D12_RESOURCE_STATES prevState =
          changeResourceState(rawList, D3D12_RESOURCE_STATE_COPY_DEST);
      rawList->CopyTextureRegion(&dstDesc, 0, 0, 0, &srcDesc, nullptr);
      changeResourceState(rawList, prevState);
    }
    UINT64 fenceValue = cmdList->end(cmdQueue);

    cmdQueue->waitCompletion(fenceValue);
    uploader[i]->Release();
    cmdList->reset();
    delete cmdList;
    delete[] data;
  }
}

void RootTable::create(const char* code) {
  char token[256];

  const char* codePos = code;
  UINT offset = UINT(ranges.size());

  while (*codePos != '\0') {
    DescriptorType type;
    UINT registerBase;
    UINT registerSpace;
    UINT numDescriptors;

    UINT tokenLengthWithSpace = findCommaOrNull(codePos);
    spaceRemoveCopy(token, codePos, tokenLengthWithSpace);
    readRegisterInfo(token, &type, &registerSpace, &registerBase,
                     &numDescriptors);

    D3D12_DESCRIPTOR_RANGE range;
    range.RangeType =
        (type == DescriptorType::SRV)   ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV
        : (type == DescriptorType::UAV) ? D3D12_DESCRIPTOR_RANGE_TYPE_UAV
                                        : D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    range.NumDescriptors = numDescriptors;
    range.BaseShaderRegister = registerBase;
    range.RegisterSpace = registerSpace;
    range.OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    // rangeArr.push_back(range);

    ranges.push_back(range);

    codePos += tokenLengthWithSpace;
  }

  param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  param.DescriptorTable.NumDescriptorRanges = UINT(ranges.size()) - offset;
  param.DescriptorTable.pDescriptorRanges = ranges.data() + offset;
}

void RootTable::bindCompute(ID3D12GraphicsCommandList* cmdList,
                            UINT paramIdx) const {
  cmdList->SetComputeRootDescriptorTable(paramIdx, tableStart);
}

void RootTable::bindGraphics(ID3D12GraphicsCommandList* cmdList,
                             UINT paramIdx) const {
  cmdList->SetGraphicsRootDescriptorTable(paramIdx, tableStart);
}

void RootPointer::create(const char* code) {
  char token[256];

  DescriptorType type;
  UINT registerBase;
  UINT registerSpace;
  UINT numDescriptors;

  UINT tokenLengthWithSpace = findCommaOrNull(code);
  spaceRemoveCopy(token, code, tokenLengthWithSpace);
  readRegisterInfo(token, &type, &registerSpace, &registerBase,
                   &numDescriptors);

  assert(numDescriptors == 1 && code[tokenLengthWithSpace] == '\0');

  param.ParameterType =
      (type == DescriptorType::SRV)   ? D3D12_ROOT_PARAMETER_TYPE_SRV
      : (type == DescriptorType::UAV) ? D3D12_ROOT_PARAMETER_TYPE_UAV
                                      : D3D12_ROOT_PARAMETER_TYPE_CBV;
  param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  param.Descriptor.ShaderRegister = registerBase;
  param.Descriptor.RegisterSpace = registerSpace;
}

void RootPointer::bindCompute(ID3D12GraphicsCommandList* cmdList,
                              UINT paramIdx) const {
  if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV)
    cmdList->SetComputeRootShaderResourceView(paramIdx, resourceAddress);
  else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV)
    cmdList->SetComputeRootUnorderedAccessView(paramIdx, resourceAddress);
  else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV)
    cmdList->SetComputeRootConstantBufferView(paramIdx, resourceAddress);
  else
    Error("fail to register a root pointer");
}

void RootPointer::bindGraphics(ID3D12GraphicsCommandList* cmdList,
                               UINT paramIdx) const {
  if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV)
    cmdList->SetGraphicsRootShaderResourceView(paramIdx, resourceAddress);
  else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV)
    cmdList->SetGraphicsRootUnorderedAccessView(paramIdx, resourceAddress);
  else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV)
    cmdList->SetGraphicsRootConstantBufferView(paramIdx, resourceAddress);
  else
    Error("fail to register a root pointer");
}

void RootConstants::create(const char* code) {
  char token[256];

  DescriptorType type;
  UINT registerBase;
  UINT registerSpace;
  UINT numDescriptors;

  UINT tokenLengthWithSpace = findCommaOrNull(code);
  spaceRemoveCopy(token, code, tokenLengthWithSpace);
  readRegisterInfo(token, &type, &registerSpace, &registerBase,
                   &numDescriptors);

  assert(type == DescriptorType::CBV);
  assert(numDescriptors == 1 && code[tokenLengthWithSpace] == '\0');

  param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  param.Constants.Num32BitValues = UINT(_32BitConsts.size());
  param.Constants.RegisterSpace = registerSpace;
  param.Constants.ShaderRegister = registerBase;
}

void RootConstants::bindCompute(ID3D12GraphicsCommandList* cmdList,
                                UINT paramIdx) const {
  cmdList->SetComputeRoot32BitConstants(paramIdx, UINT(_32BitConsts.size()),
                                        _32BitConsts.data(), 0);
}

void RootConstants::bindGraphics(ID3D12GraphicsCommandList* cmdList,
                                 UINT paramIdx) const {
  cmdList->SetGraphicsRoot32BitConstants(paramIdx, UINT(_32BitConsts.size()),
                                         _32BitConsts.data(), 0);
}

RootSignature::~RootSignature() {
  params.clear();
  SAFE_RELEASE(rootSig);
}

void RootSignature::build(D3D12_ROOT_SIGNATURE_FLAGS flags) {
  std::vector<D3D12_STATIC_SAMPLER_DESC> empty;
  build(empty, flags);
}

void RootSignature::build(
    const std::vector<D3D12_STATIC_SAMPLER_DESC>& samplerArr,
    D3D12_ROOT_SIGNATURE_FLAGS flags) {
  if (rootSig != nullptr) Error("It cannot be re-built.");

  std::vector<D3D12_ROOT_PARAMETER> paramArr(params.size());
  for (UINT i = 0; i < params.size(); ++i) {
    // std::visit : each type of data stored in varrootparam[i] is processed.
    std::visit(
        [&](auto&& arg) {
          paramArr[i] = arg.param;
          bIncludeRootTable |= arg.isRootTable();
        },
        params[i]);
  }

  D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
  rootSigDesc.NumParameters = paramArr.size();
  rootSigDesc.pParameters = paramArr.size() > 0 ? paramArr.data() : nullptr;
  rootSigDesc.NumStaticSamplers = samplerArr.size();
  rootSigDesc.pStaticSamplers =
      samplerArr.size() > 0 ? samplerArr.data() : nullptr;
  rootSigDesc.Flags = flags;

  ID3DBlob *blob = nullptr, *error = nullptr;
  D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob,
                              &error);
  if (error) {
    Error(reinterpret_cast<char*>(error->GetBufferPointer()));
  }
  ThrowFailedHR(getDevice()->get()->CreateRootSignature(
      0, blob->GetBufferPointer(), blob->GetBufferSize(),
      IID_PPV_ARGS(&rootSig)));
  blob->Release();
}

void RootSignature::bindCompute(ID3D12GraphicsCommandList* cmdList,
                                DescriptorHeap* heap) const {
  if (includeRootTable()) heap->bind(cmdList);

  cmdList->SetComputeRootSignature(rootSig);

  for (UINT i = 0; i < params.size(); ++i) {
    std::visit([cmdList, i](auto&& arg) { arg.bindCompute(cmdList, i); },
               params[i]);
  }
}

void RootSignature::bindGraphics(ID3D12GraphicsCommandList* cmdList,
                                 DescriptorHeap* heap) const {
  if (includeRootTable()) heap->bind(cmdList);

  cmdList->SetGraphicsRootSignature(rootSig);

  for (UINT i = 0; i < params.size(); ++i) {
    std::visit([cmdList, i](auto&& arg) { arg.bindGraphics(cmdList, i); },
               params[i]);
  }
}

void RenderTarget::allocateResource() {
  D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  D3D12_CLEAR_VALUE optClear = {.Format = format,
                                .Color = {0.f, 0.f, 0.f, 1.f}};
  resource = createCommittedTexture(format, width, height, depth, flag, state,
                                    &optClear);
  resourceState = state;
}

void RenderTarget::allocateDescriptor() {
  Texture::allocateDescriptor();
  if (rtv)
    rtv->assignRtv(*this);
  else
    rtv = rtvHeap->assignRtv(*this);
}

void RenderTarget::clear(CommandList* cmdList, float* clearValue) {
  float black[] = {0.0f, 0.0f, 0.0f, 1.0f};

  auto* rawList = cmdList->begin();
  {
    D3D12_RESOURCE_STATES prevState =
        changeResourceState(rawList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    rawList->ClearRenderTargetView(*rtv, clearValue ? clearValue : black, 0,
                                   nullptr);
    changeResourceState(rawList, prevState);
  }
  cmdList->end(cmdQueue);
}

RenderTarget::RenderTarget(DescriptorHeap* _srvHeap, DescriptorHeap* _rtvHeap,
                           CommandQueue* queue, DXGI_FORMAT format, UINT width,
                           UINT height)
    : Texture(_srvHeap, queue, format), rtvHeap(_rtvHeap) {
  this->width = width;
  this->height = height;
  allocateResource();
  allocateDescriptor();
}

void DepthTarget::allocateResource() {
  D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL |
                              D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
  D3D12_CLEAR_VALUE optClear = {.Format = format, .DepthStencil = {1.0, 0x11}};
  resource = createCommittedTexture(format, width, height, depth, flag, state,
                                    &optClear);
  resourceState = state;
}

void DepthTarget::allocateDescriptor() {
  if (dsv)
    dsv->assignDsv(*this);
  else
    dsv = dsvHeap->assignDsv(*this);
}

void DepthTarget::clear(CommandList* cmdList, float* clearValue) {
  auto* rawList = cmdList->begin();
  {
    D3D12_RESOURCE_STATES prevState =
        changeResourceState(rawList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    rawList->ClearDepthStencilView(*dsv, D3D12_CLEAR_FLAG_DEPTH,
                                   clearValue ? *clearValue : 1.0f, 0x00, 0,
                                   nullptr);
    changeResourceState(rawList, prevState);
  }
  cmdList->end(cmdQueue);
}

DepthTarget::DepthTarget(DescriptorHeap* dec, DescriptorHeap* _dsvHeap,
                         CommandQueue* queue, DXGI_FORMAT format, UINT width,
                         UINT height)
    : Texture(dec, queue, format), dsvHeap(_dsvHeap) {
  this->width = width;
  this->height = height;
  allocateResource();
  allocateDescriptor();
}

void ComputeTarget::allocateResource() {
  D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  // D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  resource = createCommittedTexture(format, width, height, depth, flag, state,
                                    nullptr);
  resourceState = state;
}

void ComputeTarget::allocateDescriptor() {
  Texture::allocateDescriptor();
  if (uav)
    uav->assignUav(*this);
  else
    uav = uavHeap->assignUav(*this);
}

void ComputeTarget::clear(CommandList* cmdList, float* clearValue) {
  auto* rawList = cmdList->begin();
  { assert(false); }
  cmdList->end(cmdQueue);
}

ComputeTarget::ComputeTarget(DescriptorHeap* dec, DescriptorHeap* _uavHeap,
                             CommandQueue* queue, DXGI_FORMAT format,
                             UINT width, UINT height)
    : Texture(dec, queue, format), uavHeap(_uavHeap) {
  this->width = width;
  this->height = height;
  allocateResource();
  allocateDescriptor();
}

void ReadbackBuffer::destroy() {
  SAFE_RELEASE(readbackBuffer);
  cpuAddress = nullptr;
}

void ReadbackBuffer::create(UINT64 requiredSize) {
  if (readbackBuffer != nullptr) {
    Error("destroy() must be called before re-creating.");
  }

  maxReadbackSize =
      _align(requiredSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

  readbackBuffer = createCommittedBuffer(
      maxReadbackSize, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE,
      D3D12_RESOURCE_STATE_COPY_DEST);
}

void* ReadbackBuffer::map() {
  auto range = Range(0);
  if (!cpuAddress) readbackBuffer->Map(0, &range, &cpuAddress);
  return cpuAddress;
}

void ReadbackBuffer::unmap() {
  if (cpuAddress) {
    readbackBuffer->Unmap(0, nullptr);
    cpuAddress = nullptr;
  }
}

void ReadbackBuffer::readback(ID3D12GraphicsCommandList* cmdList,
                              const dxResource& source) {
  D3D12_RESOURCE_DESC desc = source.get()->GetDesc();

  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    if (desc.Width > maxReadbackSize) {
      printf(
          "Note: The source's buffer size is bigger than current readback "
          "buffer's size, so resizing..\n");
      create(desc.Width * 2);
    }

    D3D12_RESOURCE_STATES prevState =
        source.changeResourceState(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
    cmdList->CopyBufferRegion(readbackBuffer, 0, source.get(), 0, desc.Width);
    source.changeResourceState(cmdList, prevState);
  } else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureLayout;
    UINT64 rowPureSize;   // width * bytesPerPixel
    UINT rowPitch;        // align( width * bytesPerPixel, 256 )
    UINT rows;            // height
    UINT64 copyableSize;  // (rows-1) * rowPitch + rowPureSize  ( ==
                          // _textureDataSize(desc.Format,
                          // desc.Width, desc.Height) )

    getDevice()->get()->GetCopyableFootprints(
        &desc, 0, 1, 0, &textureLayout, &rows, &rowPureSize, &copyableSize);
    rowPitch = textureLayout.Footprint.RowPitch;

    if (copyableSize > maxReadbackSize) {
      printf(
          "Note: The source's buffer size is bigger than current readback "
          "buffer's size, so resizing..\n");
      create(copyableSize * 2);
    }

    D3D12_RESOURCE_STATES prevState =
        source.changeResourceState(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

    D3D12_TEXTURE_COPY_LOCATION srcTex;
    srcTex.pResource = source.get();
    srcTex.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcTex.SubresourceIndex = 0;
    D3D12_TEXTURE_COPY_LOCATION dstBuff;
    dstBuff.pResource = readbackBuffer;
    dstBuff.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstBuff.PlacedFootprint = textureLayout;

    cmdList->CopyTextureRegion(&dstBuff, 0, 0, 0, &srcTex, nullptr);

    source.changeResourceState(cmdList, prevState);
  } else {
    assert(false);
  }
}

void dxShader::load(const char* hlslFile, const char* entryFtn,
                    const char* target) {
  std::string filename(hlslFile);
  std::wstring wfilename(filename.begin(), filename.end());

  ID3DBlob* error = nullptr;
  UINT compileFlags = 0;
#if defined(_DEBUG)
  compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
  HRESULT hr = D3DCompileFromFile(wfilename.c_str(),  // filename
                                  nullptr,            // defines
                                  nullptr,            // includes
                                  entryFtn,           // entry
                                  target,             // targetProfile
                                  compileFlags, 0,    // flag1, flag2
                                  &code, &error);

  if (FAILED(hr)) {
    if (error) {
      Error(reinterpret_cast<char*>(error->GetBufferPointer()));
    }
    printf("Can't find the file : %s\n", filename.c_str());
    throw hr;
  }
}

void GraphicsPipeline::destroy() {
  SAFE_RELEASE(pipeline);
  pRootSig = nullptr;

  numRts = 0;
  enableDepth = false;
  dscrHandles.clear();
  beginBarriers.clear();
  endBarriers.clear();

  viewport = {};
  scissorRect = {0, 0, maxTargetSize, maxTargetSize};
}

void GraphicsPipeline::build(
    const RootSignature* rootSig, D3D12_INPUT_LAYOUT_DESC inputLayout,
    D3D12_SHADER_BYTECODE vsCode, D3D12_SHADER_BYTECODE psCode, UINT numRts,
    const std::vector<BlendMode>& blendModes, DepthMode depthMode,
    const std::vector<DXGI_FORMAT>& renderFormats, DXGI_FORMAT depthFormat,
    D3D12_CULL_MODE cullMode) {
  assert(pipeline == nullptr);
  assert(renderFormats.size() == numRts);
  assert(blendModes.size() == 1 || blendModes.size() == numRts);
  enableDepth = depthMode != depth_disable;
  UINT numTargets = numRts + (enableDepth ? 1 : 0);

  pRootSig = rootSig;
  this->numRts = numRts;
  dscrHandles.resize(numTargets, nullptr);

  beginBarriers.reserve(numTargets);
  endBarriers.reserve(numTargets);

  D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
  desc.SampleMask = UINT_MAX;
  desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  desc.SampleDesc.Count = 1;

  desc.pRootSignature = pRootSig->get();
  desc.InputLayout = inputLayout;
  desc.VS = vsCode;
  desc.PS = psCode;

  desc.NumRenderTargets = numRts;
  for (UINT i = 0; i < numRts; ++i) desc.RTVFormats[i] = renderFormats[i];

  D3D12_RASTERIZER_DESC rasDesc = {};
  {
    rasDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasDesc.CullMode = cullMode;
    rasDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasDesc.DepthClipEnable = TRUE;
  }
  desc.RasterizerState = rasDesc;

  D3D12_BLEND_DESC blendDesc = {};
  {
    blendDesc.IndependentBlendEnable = blendModes.size() == 1 ? FALSE : TRUE;

    const D3D12_RENDER_TARGET_BLEND_DESC opaqueBlendDesc = {
        FALSE,
        FALSE,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,  // SrcBlend, DestBlend, BlendOp
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,  // SrcBlendAlpha, DestBlendAlpha, BlendOpAlpha
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };

    for (UINT i = 0; i < numRts; ++i) {
      if (!blendDesc.IndependentBlendEnable && i > 0) continue;

      D3D12_RENDER_TARGET_BLEND_DESC eachBlendDesc = opaqueBlendDesc;
      switch (blendModes[i]) {
        case blend_opaque:
          break;
        case blend_additive:
          eachBlendDesc.BlendEnable = TRUE;
          eachBlendDesc.SrcBlend = D3D12_BLEND_ONE;
          eachBlendDesc.DestBlend = D3D12_BLEND_ONE;
          break;
        case blend_translucent:
          eachBlendDesc.BlendEnable = TRUE;
          eachBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
          eachBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
          break;
        default:
          assert(false);
      }
      blendDesc.RenderTarget[i] = eachBlendDesc;
    }
  }
  desc.BlendState = blendDesc;

  if (enableDepth) {
    desc.DSVFormat = depthFormat;
    D3D12_DEPTH_STENCIL_DESC dsDesc = {};
    {
      dsDesc.DepthEnable = TRUE;
      dsDesc.DepthWriteMask = (depthMode == depth_readOnly)
                                  ? D3D12_DEPTH_WRITE_MASK_ZERO
                                  : D3D12_DEPTH_WRITE_MASK_ALL;
      dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
      dsDesc.StencilEnable = FALSE;
    }
    desc.DepthStencilState = dsDesc;
  }

  ThrowFailedHR(getDevice()->get()->CreateGraphicsPipelineState(
      &desc, IID_PPV_ARGS(&pipeline)));
}

void GraphicsPipeline::begin(ID3D12GraphicsCommandList* cmdList) const {
#ifdef _DEBUG
  for (UINT i = 0; i < dscrHandles.size(); ++i) {
    assert(dscrHandles[i] != nullptr);
    assert(dscrHandles[i]->getResource() != nullptr);
  }
  assert(viewport.Width > 0 && viewport.Height > 0);
#endif

  std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs(numRts);
  for (UINT i = 0; i < numRts; ++i) {
    rtvs[i] = dscrHandles[i]->getCpuHandle();
  }
  const D3D12_CPU_DESCRIPTOR_HANDLE* pDsv =
      enableDepth ? &dscrHandles[numRts]->getCpuHandle() : nullptr;

  currentCmdList = cmdList;
  currentCmdList->SetPipelineState(pipeline);

  if (pRootSig) pRootSig->bindGraphics(currentCmdList, decHeap);

  currentCmdList->OMSetRenderTargets(numRts, &rtvs[0], FALSE, pDsv);
  currentCmdList->RSSetViewports(1, &viewport);
  currentCmdList->RSSetScissorRects(1, &scissorRect);

  for (UINT i = 0; i < numRts; ++i) {
    const dxResource* rsc = dscrHandles[i]->getResource();
    if (rsc->getState() != D3D12_RESOURCE_STATE_RENDER_TARGET) {
      beginBarriers.push_back(Transition(rsc->get(), rsc->getState(),
                                         D3D12_RESOURCE_STATE_RENDER_TARGET));
      endBarriers.push_back(Transition(
          rsc->get(), D3D12_RESOURCE_STATE_RENDER_TARGET, rsc->getState()));
    }
  }
  if (enableDepth) {
    const dxResource* rsc = dscrHandles[numRts]->getResource();
    if (rsc->getState() != D3D12_RESOURCE_STATE_DEPTH_WRITE) {
      beginBarriers.push_back(Transition(rsc->get(), rsc->getState(),
                                         D3D12_RESOURCE_STATE_DEPTH_WRITE));
      endBarriers.push_back(Transition(
          rsc->get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, rsc->getState()));
    }
  }

  currentCmdList->ResourceBarrier(UINT(beginBarriers.size()),
                                  beginBarriers.data());
  beginBarriers.resize(0);

  if (doClear) {
    float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
    for (UINT i = 0; i < numRts; ++i)
      currentCmdList->ClearRenderTargetView(rtvs[i], &black[0], 0, nullptr);
    if (enableDepth)
      currentCmdList->ClearDepthStencilView(*pDsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f,
                                            0x00, 0, nullptr);
    doClear = false;
  }
}

void GraphicsPipeline::end() const {
  currentCmdList->ResourceBarrier(UINT(endBarriers.size()), endBarriers.data());
  endBarriers.resize(0);
  currentCmdList = nullptr;
}

SwapChain::~SwapChain() { SAFE_RELEASE(swapChain); }

SwapChain::BackBuffer::BackBuffer(ID3D12Resource* resource,
                                  DescriptorHeap* rtvHeap) {
  this->resource = resource;
  resourceState = D3D12_RESOURCE_STATE_COMMON;
  rtv = rtvHeap->assignRtv(*this);
}

SwapChain::BackBuffer::BackBuffer(ID3D12Resource* resource) {
  this->resource = resource;
  resourceState = D3D12_RESOURCE_STATE_COMMON;
}

void SwapChain::create(HWND hwnd, DescriptorHeap* rtvHeap,
                       CommandQueue* cmdQueue, UINT width, UINT height,
                       UINT numBackBuffers, DXGI_FORMAT bufferFormat,
                       bool allowTearing) {
  if (swapChain != nullptr)
    Error("destroy() must be called before re-creating.");

  numBuffers = numBackBuffers;
  bufferW = width;
  bufferH = height;
  this->bufferFormat = bufferFormat;
  flags = allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

  DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
  swapChainDesc.BufferCount = numBuffers;
  swapChainDesc.Width = bufferW;
  swapChainDesc.Height = bufferH;
  swapChainDesc.Format = bufferFormat;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapChainDesc.Flags = flags;
  swapChainDesc.SampleDesc.Count = 1;

  ThrowFailedHR(getDevice()->getFactory()->CreateSwapChainForHwnd(
      cmdQueue->get(), hwnd, &swapChainDesc, nullptr, nullptr,
      (IDXGISwapChain1**)(IDXGISwapChain3**)&swapChain));

  backBuffers.reserve(numBuffers);
  for (UINT i = 0; i < numBuffers; ++i) {
    ID3D12Resource* rawRsc;
    ThrowFailedHR(swapChain->GetBuffer(i, IID_PPV_ARGS(&rawRsc)));
    backBuffers.emplace_back(rawRsc, rtvHeap);
  }

  currentBufferIdx = swapChain->GetCurrentBackBufferIndex();
  currentRtv = *backBuffers[currentBufferIdx].rtv;
}

void SwapChain::present() {
  ThrowFailedHR(swapChain->Present(1, 0));
  currentBufferIdx = swapChain->GetCurrentBackBufferIndex();
  currentRtv = *backBuffers[currentBufferIdx].rtv;
}

void SwapChain::resize(UINT width, UINT height) {
  if (bufferW == width && bufferH == height) return;

  bufferW = width;
  bufferH = height;

  backBuffers.resize(0);

  ThrowFailedHR(swapChain->ResizeBuffers(numBuffers, bufferW, bufferH,
                                         bufferFormat, flags));

  for (UINT i = 0; i < numBuffers; ++i) {
    ID3D12Resource* rawRsc;
    ThrowFailedHR(swapChain->GetBuffer(i, IID_PPV_ARGS(&rawRsc)));
    backBuffers.emplace_back(rawRsc);
  }

  currentBufferIdx = swapChain->GetCurrentBackBufferIndex();
  currentRtv = *backBuffers[currentBufferIdx].rtv;
}


MeshData::MeshData(CommandQueue* cmdQueue, CommandList* cmdList,
                   const char* filePath, UINT flipX, UINT flipY, UINT flipZ,
                   bool flipTexV, bool centering, bool buildAS, bool needWire) {
  void loadOBJFile(const char* filename, std::vector<float>* vertices,
                   std::vector<UINT>* indices, bool* writeNormal,
                   bool* writeTexcoord);
  std::vector<float> vertices;  // x, y, z, nx, ny, nz, u, v
  std::vector<UINT> indices;    // i, j, k
  bool writeNormal;
  bool writeTexcoord;

  loadOBJFile(filePath, &vertices, &indices, &writeNormal, &writeTexcoord);

  if (centering) {
    float minx = HUGE_VALF, miny = HUGE_VALF, minz = HUGE_VALF;
    float maxx = -HUGE_VALF, maxy = -HUGE_VALF, maxz = -HUGE_VALF;
    for (UINT i = 0; i < vertices.size(); i += 8) {
      if (minx > vertices[i]) minx = vertices[i];
      if (maxx < vertices[i]) maxx = vertices[i];

      if (miny > vertices[i + 1]) miny = vertices[i + 1];
      if (maxy < vertices[i + 1]) maxy = vertices[i + 1];

      if (minz > vertices[i + 2]) minz = vertices[i + 2];
      if (maxz < vertices[i + 2]) maxz = vertices[i + 2];
    }

    float cx = (minx + maxx) * 0.5f;
    float cy = (miny + maxy) * 0.5f;
    float cz = (minz + maxz) * 0.5f;
    float scale = 1.0f;
    // float scale = 2.0f / fmaxf(maxx - minx, maxy - miny);

    for (UINT i = 0; i < vertices.size(); i += 8) {
      vertices[i] = (vertices[i] - cx) * scale;
      vertices[i + 1] = (vertices[i + 1] - cy) * scale;
      vertices[i + 2] = (vertices[i + 2] - cz) * scale;
    }
  }

  if (flipX) {
    for (UINT i = 0; i < vertices.size(); i += 8) {
      vertices[i] = -vertices[i] * flipX;
      vertices[i + 3] = -vertices[i + 3];
    }
  }

  if (flipY) {
    for (UINT i = 0; i < vertices.size(); i += 8) {
      vertices[i + 1] = -vertices[i + 1] * flipY;
      vertices[i + 4] = -vertices[i + 4];
    }
  }

  if (flipZ) {
    for (UINT i = 0; i < vertices.size(); i += 8) {
      vertices[i + 2] = -vertices[i + 2] * flipZ;
      vertices[i + 5] = -vertices[i + 5];
    }
  }

  if (flipTexV) {
    for (UINT i = 0; i < vertices.size(); i += 8) {
      vertices[i + 7] = 1.0f - vertices[i + 7];
    }
  }

  vtxBuff.create(sizeof(float) * vertices.size());
  idxBuff.create(sizeof(UINT) * indices.size());

  memcpy(vtxBuff.map(), vertices.data(), vtxBuff.getBufferSize());
  memcpy(idxBuff.map(), indices.data(), idxBuff.getBufferSize());
  vtxBuff.unmap(cmdQueue, cmdList);
  idxBuff.unmap(cmdQueue, cmdList);

  renderInfo.numTriangles = static_cast<UINT>(indices.size()) / 3;
  renderInfo.vtxBuffView = {vtxBuff.getGpuAddress(),
                            (UINT)vtxBuff.getBufferSize(),
                            sizeof(float) * 8};  // x, y, z, nx, ny, nz, u, v
  renderInfo.idxBuffView = {idxBuff.getGpuAddress(),
                            (UINT)idxBuff.getBufferSize(),
                            DXGI_FORMAT_R32_UINT};

  if (needWire) {
    std::vector<UINT> indices_wire;
    indices_wire.reserve(indices.size());
    {
      std::set<WireIndexDuplicate> find_duplicate;
      for (UINT i = 0; i < indices.size(); i += 3) {
        UINT idx0 = indices[i];
        UINT idx1 = indices[i + 1];
        UINT idx2 = indices[i + 2];

        if (find_duplicate.insert({idx0, idx1}).second) {
          indices_wire.push_back(idx0);
          indices_wire.push_back(idx1);
          renderInfo.numWire++;
        }
        if (find_duplicate.insert({idx1, idx2}).second) {
          indices_wire.push_back(idx1);
          indices_wire.push_back(idx2);
          renderInfo.numWire++;
        }
        if (find_duplicate.insert({idx2, idx0}).second) {
          indices_wire.push_back(idx2);
          indices_wire.push_back(idx0);
          renderInfo.numWire++;
        }
      }
    }
    wireIdxBuffer.create(sizeof(UINT) * indices_wire.size());
    memcpy(wireIdxBuffer.map(), indices_wire.data(),
           wireIdxBuffer.getBufferSize());
    wireIdxBuffer.unmap(cmdQueue, cmdList);

    renderInfo.wireIdxBuffView = {wireIdxBuffer.getGpuAddress(),
                                  (UINT)wireIdxBuffer.getBufferSize(),
                                  DXGI_FORMAT_R32_UINT};
  }
}