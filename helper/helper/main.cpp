
#include "Render.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>


#ifdef _DEBUG
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


int main() {
  //_CrtSetBreakAlloc(86);
  std::unique_ptr<Render> render = std::make_unique<Render>();
  render->init();

  _CrtDumpMemoryLeaks();
 
  return 0;
}

