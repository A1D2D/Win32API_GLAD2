/*
This is a half borrowed code from: Xeek
Original un modified code at:
https://github.com/Dav1dde/glad/blob/glad2/example/c/wgl.c
*/

#define WIN32_LEAN_AND_MEAN

#include <glad/wgl.h>
#include <glad/gl.h>
#include <windows.h>


static GLADapiproc win_GL_DlSymHandle(void* handle, const char* name) {
   if (handle == nullptr) {
      return nullptr;
   }

#if GLAD_PLATFORM_WIN32
   return (GLADapiproc)GetProcAddress((HMODULE)handle, name);
#else
   return GLAD_GNUC_EXTENSION (GLADapiproc) dlsym(handle, name);
#endif
}

typedef void* (GLAD_API_PTR* Win_Gl_ProcAddrFunc)(const char*);

struct Win_Gl_UserPTR {
   void* handle;
   Win_Gl_ProcAddrFunc gl_get_proc_address_ptr;
};

static GLADapiproc win_GL_getProcAddrFunc(void* vuserptr, const char* name) {
   struct Win_Gl_UserPTR userptr = *(struct Win_Gl_UserPTR*)vuserptr;
   GLADapiproc result = nullptr;

   if (userptr.gl_get_proc_address_ptr != nullptr) {
      result = GLAD_GNUC_EXTENSION (GLADapiproc)userptr.gl_get_proc_address_ptr(name);
   }
   if (result == nullptr) {
      result = win_GL_DlSymHandle(userptr.handle, name);
   }

   return result;
}

static void* win_GL_GetDlOpenHandle(const char* lib_names[], int length) {
   void* handle = nullptr;
   int i;

   for (i = 0; i < length; ++i) {
      #if GLAD_PLATFORM_WIN32
         #if GLAD_PLATFORM_UWP
            size_t buffer_size = (strlen(lib_names[i]) + 1) * sizeof(WCHAR);
            LPWSTR buffer = (LPWSTR) malloc(buffer_size);
            if (buffer != nullptr) {
               int ret = MultiByteToWideChar(CP_ACP, 0, lib_names[i], -1, buffer, buffer_size);
               if (ret != 0) handle = (void*) LoadPackagedLibrary(buffer, 0);
               free((void*) buffer);
            }
         #else
            handle = (void*)LoadLibraryA(lib_names[i]);
         #endif
      #else
         handle = dlopen(lib_names[i], RTLD_LAZY | RTLD_LOCAL);
      #endif

      if (handle != nullptr) {
         return handle;
      }
   }

   return nullptr;
}

static void* win_GL_LoaderHandle = nullptr;
static void* win_GL_DlOpenHandle() {
   #if GLAD_PLATFORM_APPLE
      static const char *NAMES[] = {
         "../Frameworks/OpenGL.framework/OpenGL",
         "/Library/Frameworks/OpenGL.framework/OpenGL",
         "/System/Library/Frameworks/OpenGL.framework/OpenGL",
         "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL"
      };
   #elif GLAD_PLATFORM_WIN32
      static const char* NAMES[] = {"opengl32.dll"};
   #else
      static const char *NAMES[] = {
   #if defined(__CYGWIN__)
      "libGL-1.so",
   #endif
      "libGL.so.1",
      "libGL.so"
      };
   #endif

   if (win_GL_LoaderHandle == nullptr) {
      win_GL_LoaderHandle = win_GL_GetDlOpenHandle(NAMES, sizeof(NAMES) / sizeof(NAMES[0]));
   }

   return win_GL_LoaderHandle;
}

static struct Win_Gl_UserPTR win_Gl_BuildUserPTR(void* handle) {
   struct Win_Gl_UserPTR userPTR{};

   userPTR.handle = handle;
#if GLAD_PLATFORM_APPLE || defined(__HAIKU__)
   userPTR.gl_get_proc_address_ptr = nullptr;
#elif GLAD_PLATFORM_WIN32
   userPTR.gl_get_proc_address_ptr =
         (Win_Gl_ProcAddrFunc)win_GL_DlSymHandle(handle, "wglGetProcAddress");
#else
   userPTR.gl_get_proc_address_ptr =
        (Win_Gl_ProcAddrFunc) Win_GL_DlSymHandle(handle, "glXGetProcAddressARB");
#endif

   return userPTR;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static const TCHAR window_classname[] = "SampleWndClass";
static const TCHAR window_title[] = "[glad] WGL";
static const POINT window_location = {CW_USEDEFAULT, 0};
static const SIZE window_size = {1024, 768};
static const GLfloat clear_color[] = {0.0f, 0.0f, 1.0f, 1.0f};


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
   UNREFERENCED_PARAMETER(hPrevInstance)
   UNREFERENCED_PARAMETER(lpCmdLine)

   WNDCLASSEX wcex = {};
   wcex.cbSize = sizeof(WNDCLASSEX);
   wcex.style = CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc = WndProc;
   wcex.hInstance = hInstance;
   wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
   wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
   wcex.lpszClassName = window_classname;

   ATOM wndclass = RegisterClassEx(&wcex);

   HWND hWnd = CreateWindow(MAKEINTATOM(wndclass), window_title, WS_OVERLAPPEDWINDOW, window_location.x, window_location.y, window_size.cx, window_size.cy,
                            nullptr, nullptr, hInstance, nullptr);

   if (!hWnd) {
      return -1;
   }

   HDC hdc = GetDC(hWnd);
   if (hdc == nullptr) {
      DestroyWindow(hWnd);
      return -1;
   }

   PIXELFORMATDESCRIPTOR pfd = {};
   pfd.nSize = sizeof(pfd);
   pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);  // Set the size of the PFD to the size of the class
   pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;   // Enable double buffering, opengl support and drawing to a window
   pfd.iPixelType = PFD_TYPE_RGBA; // Set our application to use RGBA pixels
   pfd.cColorBits = 32;        // Give us 32 bits of color information (the higher, the more colors)
   pfd.cDepthBits = 32;        // Give us 32 bits of depth information (the higher, the more depth levels)
   pfd.iLayerType = PFD_MAIN_PLANE;    // Set the layer of the PFD
   int format = ChoosePixelFormat(hdc, &pfd);
   if (format == 0 || SetPixelFormat(hdc, format, &pfd) == FALSE) {
      ReleaseDC(hWnd, hdc);
      DestroyWindow(hWnd);
      return -1;
   }

   // Create and enable a temporary (helper) opengl context:
   HGLRC temp_context = nullptr;
   if (nullptr == (temp_context = wglCreateContext(hdc))) {
      ReleaseDC(hWnd, hdc);
      DestroyWindow(hWnd);
      return -1;
   }
   wglMakeCurrent(hdc, temp_context);

   gladLoaderLoadWGL(hdc);

   int attributes[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
      WGL_CONTEXT_MINOR_VERSION_ARB, 2,
      WGL_CONTEXT_FLAGS_ARB,
      WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
      0
   };

   HGLRC opengl_context = nullptr;
   if (nullptr == (opengl_context = wglCreateContextAttribsARB(hdc, nullptr, attributes))) {
      ReleaseDC(hWnd, hdc);
      DestroyWindow(hWnd);
      return -1;
   }
   wglMakeCurrent(nullptr, nullptr);
   wglMakeCurrent(hdc, opengl_context);

   void* handle;
   struct Win_Gl_UserPTR userptr{};
   handle = win_GL_DlOpenHandle();
   if (handle) {
      userptr = win_Gl_BuildUserPTR(handle);
   }

   GladGLContext context;
   if (!gladLoadGLContextUserPtr(&context, win_GL_getProcAddrFunc, &userptr)) {
      wglMakeCurrent(nullptr, nullptr);
      wglDeleteContext(opengl_context);
      ReleaseDC(hWnd, hdc);
      DestroyWindow(hWnd);
      return -1;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   bool should_quit = false;
   MSG msg = {};
   while (!should_quit) {

      while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);

         if (msg.message == WM_QUIT || (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE))
            should_quit = true;
      }

      context.ClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
      context.Clear(GL_COLOR_BUFFER_BIT);

      SwapBuffers(hdc);
   }


   if (opengl_context) wglDeleteContext(opengl_context);
   if (hdc) ReleaseDC(hWnd, hdc);
   if (hWnd) DestroyWindow(hWnd);

   return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch (uMsg) {
      case WM_QUIT:
      case WM_DESTROY:
      case WM_CLOSE:
         PostQuitMessage(0);
         break;
      default:
         return DefWindowProc(hWnd, uMsg, wParam, lParam);
   }

   return 0;
}
