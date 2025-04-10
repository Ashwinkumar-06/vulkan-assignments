// header files
#include <Windows.h>
//#include <climits>
//#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Vulkan related header files
#define VK_USE_PLATFORM_WIN32_KHR // define the current Vulkan platform
#include <vulkan/vulkan.h>        // you must define platform before including this file (Windows / Linux / macOS / iOS / Android / <other>)

#include "VK.h"

// Vulkan related libraries
#pragma comment(lib, "vulkan-1.lib")

// macros
#define WIN_WIDTH  800
#define WIN_HEIGHT 600
#define WIN_TITLE  TEXT("Vulkan - Vulkan BlueScreen - Step (20)")
#define BORDER     "****************************************************************************************************\n"

// global function declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// global variable declarations
const char     *gpszAppName  = "ARTR";
HWND            ghwnd        = NULL;
BOOL            gbFullscreen = FALSE;
BOOL            gbActive     = FALSE;
WINDOWPLACEMENT wpPrev;
DWORD           dwStyle;

// for file IO
FILE *gpFILE = NULL;

// Vulkan related global variables
// Instance extension related variables
uint32_t enabledInstanceExtensionCount = 0;

// 1. VK_KHR_SURFACE_EXTENSION_NAME
// 2. VK_KHR_WIN32_SURFACE_EXTENSION_NAME
const char *enabledInstanceExtensionNames_Array[2]; 

// Vulkan Instance
VkInstance vkInstance = VK_NULL_HANDLE;

// Vulkan Presentation Surface
VkSurfaceKHR vkSurfaceKHR = VK_NULL_HANDLE;

// Vulkan Physical Device related global variables
VkPhysicalDevice                 vkPhysicalDevice_Selected         = VK_NULL_HANDLE;
uint32_t                         graphicsQueueFamilyIndex_Selected = UINT32_MAX;
uint32_t                         physicalDeviceCount               = 0;
VkPhysicalDevice                *vkPhysicalDevice_Array            = NULL;
VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties;

// Vulkan Device Extension related variables
uint32_t enabledDeviceExtensionCount = 0;

// 1. VK_KHR_SWAPCHAIN_EXTENSION_NAME
const char *enabledDeviceExtensionNames_Array[1]; // for Vulkan on macOS, this array will change size to 2 and for Ray-tracing, it will be of minimum size 8.

// Vulkan Logical Device
VkDevice vkDevice = VK_NULL_HANDLE;

// Vulkan Device Queue
VkQueue vkQueue = VK_NULL_HANDLE;

// Color format and color Space
VkFormat        vkFormat_color  = VK_FORMAT_UNDEFINED;
VkColorSpaceKHR vkColorSpaceKHR = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

// Present Mode
VkPresentModeKHR  vkPresentModeKHR = VK_PRESENT_MODE_FIFO_KHR;

// Swapchain related Global variable
int winWidth = WIN_WIDTH;
int winHeight = WIN_HEIGHT;

VkSwapchainKHR vkSwapchainKHR = VK_NULL_HANDLE;

VkExtent2D vkExtent2D_Swapchain;

// Swapchain Images and Swapchain ImageViews Related Variables
uint32_t swapchainImageCount = UINT32_MAX;

VkImage * swapchainImage_Array  = NULL;

VkImageView * swapchainImageView_Array = NULL;


// Command Pool 
VkCommandPool vkCommandPool = VK_NULL_HANDLE;

// Commad Buffer
VkCommandBuffer * vkCommandBuffer_Array = NULL;

// RenderPass
VkRenderPass vkRenderPass = VK_NULL_HANDLE;

// Framebuffer
VkFramebuffer * vkFramebuffer_Array = NULL;

/*

    Step 1 : Globaly declare an Array of Fences of pointer type VkFence additionally declare 2 semaphore objects of type VkSemaphore

*/
VkSemaphore vkSemaphore_BackBuffer = VK_NULL_HANDLE;
VkSemaphore vkSemaphore_RenderComplete = VK_NULL_HANDLE;
VkFence* vkFence_Array = NULL;

// Clear color values
VkClearColorValue vkClearColorValue;

BOOL bInitialized = FALSE;

uint32_t currentImageIndex = UINT32_MAX;

// entry-point function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
    // function prototypes
    VkResult initialize(void);
    VkResult display(void);
    void update(void);
    void uninitialize();

    // local variable declarations
    MSG        msg; 
    WNDCLASSEX wndclass;
    TCHAR      szAppName[255];
    HWND       hwnd     = NULL;
    BOOL       bDone    = FALSE;
    VkResult   vkResult = VK_SUCCESS; 

    // code
    // open the log file
    gpFILE = fopen("Log.txt", "w");
    if(gpFILE == NULL)
    {
        MessageBox(NULL, TEXT("WinMain() : fopen() failed to open the log file."), TEXT("Error"), MB_OK | MB_ICONERROR);
        exit(EXIT_FAILURE);
    }
    else
    {
        fprintf(gpFILE, "WinMain() : Program started successfully.\n");
        fprintf(gpFILE, BORDER);
    }

    // copy the global app name into the local app name
    wsprintf(szAppName, TEXT("%s"), gpszAppName);

    // zero-out the window class
    ZeroMemory(&wndclass, sizeof(WNDCLASSEX));

    // fill the window class
    wndclass.cbSize        = sizeof(WNDCLASSEX);
    wndclass.style         = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON));
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpfnWndProc   = WndProc;
    wndclass.lpszClassName = szAppName;
    wndclass.lpszMenuName  = NULL;
    wndclass.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON));

    // register the window class
    RegisterClassEx(&wndclass);

    // create the main window in memory
    hwnd = CreateWindowEx(
                            WS_EX_APPWINDOW, 
                            szAppName, 
                            WIN_TITLE, 
                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
                            (GetSystemMetrics(SM_CXSCREEN) - WIN_WIDTH) / 2,
                            (GetSystemMetrics(SM_CYSCREEN) - WIN_HEIGHT) / 2,
                            WIN_WIDTH,
                            WIN_HEIGHT,
                            HWND_DESKTOP,
                            NULL,
                            hInstance,
                            NULL
                         );

    if(!hwnd)
    {
        MessageBox(NULL, TEXT("WinMain() : CreateWindowEx() failed."), TEXT("Error"), MB_OK | MB_ICONERROR);
        exit(EXIT_FAILURE);
    }

    // copy the local window handle into the global window handle
    ghwnd = hwnd;

    // initialization
    vkResult = initialize();
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "WinMain() : initialize() failed.\n");
        DestroyWindow(hwnd);
        hwnd = NULL;
    }
    else
    {
        fprintf(gpFILE, BORDER);
        fprintf(gpFILE, "WinMain() : initialize() succeeded.\n");
    }

    // show the window
    ShowWindow(hwnd, iCmdShow);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    // gameloop
    while(bDone == FALSE)
    {
        ZeroMemory(&msg, sizeof(MSG));
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
            {
                bDone = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else if(gbActive)
        {
            // Render
            display();

            // Update
            update();
        }
    }

    // uninitialization
    uninitialize();
    
    // un-register the window class
    UnregisterClass(szAppName, hInstance);

    return((int)msg.wParam);
}

// callback function
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    // function declarations
    void resize(int width, int height);
    void ToggleFullscreen(void);

    // code
    switch(iMsg)
    {
    case WM_CREATE:
        memset(&wpPrev, 0, sizeof(WINDOWPLACEMENT));
        wpPrev.length = sizeof(WINDOWPLACEMENT);
        break;
    case WM_SETFOCUS:
        gbActive = TRUE;
        break;
    case WM_KILLFOCUS:
        gbActive = FALSE;
        break;
    case WM_ERASEBKGND:
        return(0);
    case WM_SIZE:
        resize(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_KEYDOWN:
        switch(LOWORD(wParam))
        {
        case VK_ESCAPE:
            DestroyWindow(hwnd);
            break;
        default:
            break;
        }
        break;
    case WM_CHAR:
        switch(LOWORD(wParam))
        {
        case 'F':
        case 'f':
            ToggleFullscreen();
            break;
        default:
            break;
        }
        break;     
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        break;
    }

    return(DefWindowProc(hwnd, iMsg, wParam, lParam));
}

// toggle fullscreen
void ToggleFullscreen(void)
{
    // local variables
    MONITORINFO monitorInfo = { sizeof(MONITORINFO) };

    // code
    // if window isn't in fullscreen mode
    if(gbFullscreen == FALSE)
    {
        dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
        if(dwStyle & WS_OVERLAPPEDWINDOW)
        {
            if(GetWindowPlacement(ghwnd, &wpPrev) && 
               GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &monitorInfo))
            {
                SetWindowLong(ghwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(
                             ghwnd, 
                             HWND_TOP, 
                             monitorInfo.rcMonitor.left, 
                             monitorInfo.rcMonitor.top,
                             monitorInfo.rcMonitor.right  - monitorInfo.rcMonitor.left,
                             monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                             SWP_NOZORDER | SWP_FRAMECHANGED
                            );

                ShowCursor(FALSE);

                gbFullscreen = TRUE;
            }
        }
    }
    else // window is already in fullscreen mode
    {
        SetWindowPlacement(ghwnd, &wpPrev);
        SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPos(
                     ghwnd, 
                     HWND_TOP, 
                     0, 
                     0, 
                     0, 
                     0, 
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
                    );
        
        ShowCursor(TRUE);

        gbFullscreen = FALSE;
    }
}

VkResult initialize(void)
{
    // function declarations
    VkResult createVulkanInstance(void);
    VkResult getSupportedSurface(void);
    VkResult getPhysicalDevice(void);
    VkResult printVkInfo(void);
    VkResult createVulkanDevice(void);
    void getDeviceQueue(void);
    VkResult getPhysicalDeviceSurfaceFormatAndColorSpace(void);
    VkResult getPhysicalDevicePresentMode(void);
    VkResult createSwapchain(VkBool32);
    VkResult CreateImagesAndImageViews(void);
    VkResult CreateCommandPool(void);
    VkResult CreateCommandBuffers(void);
    VkResult CreateRenderPass(void);
    VkResult CreateFramebuffers(void);
    VkResult CreateSemaphores(void);
    VkResult CreateFences(void);
    VkResult BuildCommandBuffers(void);

    // variable declarations
    VkResult vkResult = VK_SUCCESS;

    // code
    // STEP 3 : Create Vulkan instance
    vkResult = createVulkanInstance();
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "initialize() : createVulkanInstance() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "initialize() : createVulkanInstance() succeeded.\n");
    }

    // STEP 4 : Create Vulkan Presentation Surface
    vkResult = getSupportedSurface();
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "initialize() : getSupportedSurface() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "initialize() : getSupportedSurface() succeeded.\n");
    }

    // STEP 5 : Enumerate and select required physical device and its queue family index
    vkResult = getPhysicalDevice();
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "initialize() : getPhysicalDevice() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "initialize() : getPhysicalDevice() succeeded.\n");
    }

    // STEP 6 : Print Vulkan information
    vkResult = printVkInfo();
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "initialize() : printVkInfo() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "initialize() : printVkInfo() succeeded.\n");
    }

    // STEP 8 : Create Vulkan (Logical) Device
    vkResult = createVulkanDevice();
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "initialize() : createVulkanDevice() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "initialize() : createVulkanDevice() succeeded.\n");
    }

    // STEP 9 : Create Vulkan Device Queue
    getDeviceQueue();

    // Create Swapchain
    vkResult = createSwapchain(VK_FALSE);
    if(vkResult != VK_SUCCESS)
    {
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // return hard-coded failure
        fprintf(gpFILE, "initialize() : createSwapchain() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "initialize() : createSwapchain() succeeded.\n");
    }

    // Create Vulkan Images And Image Views
     vkResult = CreateImagesAndImageViews();
     if(vkResult != VK_SUCCESS)
     {
         fprintf(gpFILE, "initialize() : CreateImagesAndImageViews() failed (%d).\n", vkResult);
         return(vkResult);
     }
     else
     {
         fprintf(gpFILE, "initialize() : CreateImagesAndImageViews() succeeded.\n");
     }



     // Create Commad Pool
     vkResult = CreateCommandPool();

     if(vkResult != VK_SUCCESS)
     {
         fprintf(gpFILE, "initialize() : CreateCommandPool() failed (%d).\n", vkResult);
         return(vkResult);
     }
     else
     {
         fprintf(gpFILE, "initialize() : CreateCommandPool() succeeded.\n");
     }


    // Create Command Buffer 
    vkResult = CreateCommandBuffers();
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "initialize() : CreateCommandBuffers() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "initialize() : CreateCommandBuffers() succeeded.\n");
    }

    // Create Render Pass
    vkResult = CreateRenderPass();
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "initialize() : CreateRenderPass() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "initialize() : CreateRenderPass() succeeded.\n");
    }


    vkResult = CreateFramebuffers();
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "initialize() : CreateFramebuffers() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "initialize() : CreateFramebuffers() succeeded.\n");
    }

    // Create Semaphores

    vkResult = CreateSemaphores();
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "initialize() : CreateSemaphores() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "initialize() : CreateSemaphores() succeeded.\n");
    }

    // Create Fences

    vkResult =  CreateFences(); 
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "initialize() : CreateFences() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "initialize() : CreateFences() succeeded. %d\n", vkResult);
    }

    // Initialize clear color values
    memset((void*)&vkClearColorValue, 0, sizeof(VkClearColorValue));
    

    /*
        Below code is analogus to clearcolor
    */
    vkClearColorValue.float32[0] = 0.0f;
    vkClearColorValue.float32[1] = 0.0f;
    vkClearColorValue.float32[2] = 1.0f;
    vkClearColorValue.float32[2] = 1.0f; 

    vkResult =  BuildCommandBuffers();
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "initialize() : BuildCommandBuffers() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "initialize() : BuildCommandBuffers() succeeded. %d\n", vkResult);
    }

    // *********************************** Initialization is completed ***********************************

    bInitialized = TRUE;

    return(vkResult);
}

void resize(int width, int height)
{
    // code
    if(height <= 0)
    {
        height = 1;
    }
}

VkResult display(void)
{
    // Variable declaration
    VkResult vkResult = VK_SUCCESS;


    // code
    // if control comes here before initialization gets completed then return FALSE
    if(bInitialized == FALSE)
    {
        fprintf(gpFILE, "Display () : Initialization yet not completed\n");
        return (VkResult)VK_FALSE;
    }

    // Aquire Index of next swapchain Image
    vkResult = vkAcquireNextImageKHR(
                                        vkDevice,
                                        vkSwapchainKHR, 
                                        UINT64_MAX,                             // 3rd parameter  : Time out parameter : in Nanometer (here we are waiting for swapchain if we not got swapchain then it returns VK_PENDING)
                                        vkSemaphore_BackBuffer, 
                                        VK_NULL_HANDLE, 
                                        &currentImageIndex
                                    ); 

    if(vkResult != VK_SUCCESS){
        fprintf(gpFILE, "Display () : vkAquireNextImageKHR failed \n");
        return vkResult;
    }

    // use fence to allow host to wait for completion of execution of previout command buffer

    vkResult = vkWaitForFences(
                                vkDevice,
                                1,                                      // kiti fences sathi thambayach aahe
                                &vkFence_Array[currentImageIndex],      // konta fence
                                VK_TRUE,                                // I will wait for all fences to gets signaled
                                UINT64_MAX                       
    );

    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "Display () : vkWaitForFences failed \n");
        return vkResult;
    }

    // Now make ready the fences for next execution of command buffer

    vkResult = vkResetFences(
                                vkDevice,                                 // kiti fences reset karayach aahe
                                1, 
                                &vkFence_Array[currentImageIndex]
    );

    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "Display () : vkResetFencec failed \n");
        return vkResult;
    }

    // one of the member of VkSubmitInfo structure parameter requrires array of pipeline stages we have only one of completion of color attachment output still we need 1 member array

    const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    // Declare memset and initialize VkSubmitInfo Structure
    VkSubmitInfo vkSubmitInfo;

    memset((void *)&vkSubmitInfo, 0, sizeof(VkSubmitInfo));

    vkSubmitInfo.sType                  =    VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkSubmitInfo.pNext                  =    NULL;
    vkSubmitInfo.pWaitDstStageMask      =    &waitDstStageMask;
    vkSubmitInfo.waitSemaphoreCount     =    1;
    vkSubmitInfo.pWaitSemaphores        =    &vkSemaphore_BackBuffer;
    vkSubmitInfo.commandBufferCount     =    1;
    vkSubmitInfo.pCommandBuffers        =    &vkCommandBuffer_Array[currentImageIndex];
    vkSubmitInfo.signalSemaphoreCount   =    1;
    vkSubmitInfo.pSignalSemaphores      =    &vkSemaphore_RenderComplete;

    // Now submit above work to the queue
    vkResult = vkQueueSubmit(
                                vkQueue,
                                1,
                                &vkSubmitInfo,
                                vkFence_Array[currentImageIndex]
    );

    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "Display () : vkQueueSubmit failed \n");
        return vkResult;
    }

    // we are going to present rendered the image after declaring , and initializing VkPresentInfoKHR structure
    VkPresentInfoKHR vkPresentInfoKHR;

    memset((void*)&vkPresentInfoKHR, 0, sizeof(VkPresentInfoKHR));

    vkPresentInfoKHR.sType                  = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    vkPresentInfoKHR.pNext                  = NULL;
    vkPresentInfoKHR.swapchainCount         = 1;
    vkPresentInfoKHR.pSwapchains            = &vkSwapchainKHR;
    vkPresentInfoKHR.waitSemaphoreCount     = 1;
    vkPresentInfoKHR.pWaitSemaphores        = &vkSemaphore_RenderComplete;
    vkPresentInfoKHR.pImageIndices          = &currentImageIndex;
    //vkPresentInfoKHR.pResults               = ;

    // Now present the queue
    vkResult = vkQueuePresentKHR(
                                    vkQueue,
                                    &vkPresentInfoKHR
    );


    return(vkResult);
}

void update(void)
{
    // code
}

void uninitialize(void)
{
    // function declarations
    void ToggleFullscreen(void);

    // code
    // if application is exitting in fullscreen
    if(gbFullscreen == TRUE)
    {
        ToggleFullscreen();
        gbFullscreen = FALSE;
    }

    if(gpFILE)
    {
        fprintf(gpFILE, BORDER);
    }

    /*
     * no need to destroy / uninitialize device queue 
     */

    // Destroy Vulkan device
    if(vkDevice)
    {
        // Before destroying the device, ensure that all operations on that device are finished. Till then, wait on that device.
        vkDeviceWaitIdle(vkDevice);

        fprintf(gpFILE, "uninitialize() : vkDeviceWaitIdle() is done.\n");
    }

    /*

        Step 7 : loop which swapchainImageCount as counter destroy fence array objects using vkDestroyFence() and then actually free the allocated fences array by using free.

    */

    fprintf(gpFILE, "uninitialize() : vkDestroyFence() is done.\n");

    for(uint32_t i = 0; i < swapchainImageCount; i++)
    {
        vkDestroyFence(vkDevice, vkFence_Array[i], NULL);
        fprintf(gpFILE, "uninitialize() : vkDestroyFence() is done.\n");
        
    }

    if(vkFence_Array)
    {
        free(vkFence_Array);
        fprintf(gpFILE, "uninitialize() : vkFence_Array() is freed.\n");
        vkFence_Array = NULL;
    }

    if(vkSemaphore_RenderComplete)
    {
        vkDestroySemaphore(vkDevice, vkSemaphore_RenderComplete, NULL);
        fprintf(gpFILE, "uninitialize() : vkSemaphore_RenderComplete is destroyed.\n");
        vkSemaphore_RenderComplete = VK_NULL_HANDLE;
    }

    if(vkSemaphore_BackBuffer)
    {
        vkDestroySemaphore(vkDevice, vkSemaphore_BackBuffer, NULL);
        fprintf(gpFILE, "uninitialize() : vkSemaphore_BackBuffer is destroyed.\n");
        vkSemaphore_BackBuffer = VK_NULL_HANDLE;
    }

    //
    for (uint32_t i=0; i<swapchainImageCount; i++)
    {
        vkDestroyFramebuffer(vkDevice, vkFramebuffer_Array[i], NULL);
        fprintf(gpFILE, "vkFramebuffer_Array uninitialized [%d].\n", i);
    }

    if(vkFramebuffer_Array)
    {
        free(vkFramebuffer_Array);
        vkFramebuffer_Array = NULL;
        fprintf(gpFILE, "uninitialize() : vkFramebuffer_Array is done.\n");
    }

    if(vkRenderPass)
    {
        vkDestroyRenderPass(vkDevice, vkRenderPass, NULL);
        vkRenderPass = VK_NULL_HANDLE;
        fprintf(gpFILE, "uninitialize() : vkDestroyRenderPass is done.\n");
    }

    for (uint32_t i=0; i<swapchainImageCount; i++)
    {
        vkFreeCommandBuffers(vkDevice, vkCommandPool, 1, &vkCommandBuffer_Array[i]);
        fprintf(gpFILE, "vkCommandBuffer_Array uninitialized [%d].\n", i);
    }

    if(vkCommandBuffer_Array)
    {
        free(vkCommandBuffer_Array);
        vkCommandBuffer_Array = NULL;
        fprintf(gpFILE, "uninitialize() : vkCommandBuffer_Array is done.\n");
    }

    // Destroy command pool
    if(vkCommandPool)
    {
        vkDestroyCommandPool(vkDevice, vkCommandPool, NULL);
        vkCommandPool = VK_NULL_HANDLE;
        fprintf(gpFILE, "uninitialize() : vkCommandPool is done.\n");
    }

    /*
    *
       Step 9 : In uninitialize, Destroy image Views From ImageViews Array in a loop by using vkDestroyImageViewsArray().
    *
    */
   for(uint32_t i = 0; i < swapchainImageCount; i++)
   {
       vkDestroyImageView(vkDevice, swapchainImageView_Array[i], NULL);
       fprintf(gpFILE, "swapchainImageView_Array uninitialized [%d].\n", i);
   }   


   /*
    *
       Step 10 : In Uninitialize Now Actually Free the imageView Array using Free
    *
    */
   if(swapchainImageView_Array)
   {
       free(swapchainImageView_Array);
       swapchainImageView_Array = NULL;
       fprintf(gpFILE, "swapchainImageView_Array free .\n");
   }

    
   

//     /*
//     *
//        Step 7 : First destroy swapchain images First destroy Swapchain images from the swapchain images Array in a loop 
//                     using vkDestroyImage() API
//     *
//     */
//    for(uint32_t i = 0; i < swapchainImageCount; i++)
//    {
//         vkDestroyImage(vkDevice, swapchainImage_Array[i], NULL);
//         fprintf(gpFILE, "swapchainImage_Array uninitialized [%d].\n", i);        
//    }


   /*
    *
       Step 8 : Now Actually free the swapchain image Arrary
    *
    */
   
   if(swapchainImage_Array)
   {
        free(swapchainImage_Array);
        swapchainImage_Array = NULL;
        fprintf(gpFILE, "swapchainImage_Array free .\n");
   }




    // Destroy Swapchain 

    if(vkSwapchainKHR)
    {
        vkDestroySwapchainKHR(vkDevice, vkSwapchainKHR, NULL);
        vkSwapchainKHR = VK_NULL_HANDLE;
        fprintf(gpFILE, "uninitialize() : VkDestroySwapchainKHR() is done.\n");
    }


    if(vkDevice)
    {
        // finally, destroy it
        vkDestroyDevice(
                        vkDevice, // [in] Vulkan device handle
                        NULL      // [in, optional] pointer to a custom memory allocator
                    );

        fprintf(gpFILE, "uninitialize() : vkDestroyDevice() succeeded.\n");

        vkDevice = VK_NULL_HANDLE;
    }
    
    /*
     * no need to destroy selected physical device
     */
    
    // Destroy the VkSurfaceKHR object
    if(vkSurfaceKHR)
    {
        vkDestroySurfaceKHR(
                            vkInstance,   // [in] Vulkan instance handle
                            vkSurfaceKHR, // [in] Vulkan presentation surface handle
                            NULL          // [in, optional] pointer to a custom memory allocator (NULL means use a default memory allocator)
                           );

        vkSurfaceKHR = VK_NULL_HANDLE;

        fprintf(gpFILE, "uninitialize() : vkDestroySurfaceKHR() succeeded.\n");
    }

    // Destroy the VkInstance
    if(vkInstance)
    {
        vkDestroyInstance(
                          vkInstance, // [in] Vulkan instance handle
                          NULL        // [in, optional] pointer to a custom memory allocator (NULL means use a default memory allocator)
                         );

        vkInstance = VK_NULL_HANDLE;

        fprintf(gpFILE, "uninitialize() : vkDestroyInstance() succeeded.\n");
    }

    // Destroy window
    if(ghwnd)
    {
        DestroyWindow(ghwnd);
        ghwnd = NULL;
    }

    // close the log file
    if(gpFILE)
    {
        fprintf(gpFILE, "uninitialize() : Program ended successfully.\n");
        fclose(gpFILE);
        gpFILE = NULL;
    }
}

// ---------------------------------------------------------------------------------------------------------------------------------------------
// -                                                    Vulkan related function definitions                                                    -
// ---------------------------------------------------------------------------------------------------------------------------------------------
VkResult createVulkanInstance(void)
{
    // function declarations
    VkResult fillInstanceExtensionNames(void);

    // variable declarations
    VkResult vkResult = VK_SUCCESS;

    // code
    /*
     * sub-step 1 : as explained before, fill and initialize required
     *              extension names and count global variables
     */
    vkResult = fillInstanceExtensionNames();
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "createVulkanInstance() : fillInstanceExtensionNames() failed.\n");
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "createVulkanInstance() : fillInstanceExtensionNames() succeeded.\n");
    }

    /*
     * sub-step 2 : initialize struct VkApplicationInfo
     */
    VkApplicationInfo vkApplicationInfo;
    memset(&vkApplicationInfo, 0, sizeof(VkApplicationInfo));

    vkApplicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO; // structure type (VkStructureType)
    vkApplicationInfo.pNext              = NULL;                               // pointer to a structure extending this structure (linked list)
    vkApplicationInfo.pApplicationName   = gpszAppName;                        // application name (can be anything, but to be meaningful, we will use the global app name)
    vkApplicationInfo.applicationVersion = 1;                                  // can be anything, we will just use 1 (developer-supplied version)
    vkApplicationInfo.pEngineName        = gpszAppName;                        // engine name (again, can be anything, but to be meaningful, we will use the global app name)
    vkApplicationInfo.engineVersion      = 1;                                  // can be anything, we will just use 1 (developer-supplied version)
    vkApplicationInfo.apiVersion         = VK_API_VERSION_1_4;                 // must be the highest Vulkan API Version

    /*
     * sub-step 3 : initialize struct VkInstanceCreateInfo by using
     *              information from sub-step 1 and sub-step 2
     */
    VkInstanceCreateInfo vkInstanceCreateInfo;
    memset(&vkInstanceCreateInfo, 0, sizeof(VkInstanceCreateInfo));

    vkInstanceCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // structure type (VkStructureType)
    vkInstanceCreateInfo.pNext                   = NULL;                                   // pointer to a structure extending this structure (linked list)
    vkInstanceCreateInfo.pApplicationInfo        = &vkApplicationInfo;                     // pointer to a VkApplicationInfo structure 
    vkInstanceCreateInfo.enabledExtensionCount   = enabledInstanceExtensionCount;          // number of enabled Vulkan instance extensions
    vkInstanceCreateInfo.ppEnabledExtensionNames = enabledInstanceExtensionNames_Array;    // array of enabled Vulkan instance extension names

    /*
     * sub-step 4 : call vkCreateInstance() to get VkInstance in a
     *              global variable and do error checking
     */
    vkResult = vkCreateInstance(
                                &vkInstanceCreateInfo, // [in] pointer to a VkInstanceCreateInfo structure
                                NULL,                  // [in, optional] pointer to a custom memory allocator (NULL means use a default memory allocator)
                                &vkInstance            // [out] pointer to a VkInstance handle  
                               );
    
    if(vkResult == VK_ERROR_INCOMPATIBLE_DRIVER)
    {
        fprintf(gpFILE, "createVulkanInstance() : vkCreateInstance() failed due to incompatible driver (%d).\n", vkResult);
        return(vkResult);
    }
    else if(vkResult == VK_ERROR_EXTENSION_NOT_PRESENT)
    {
        fprintf(gpFILE, "createVulkanInstance() : vkCreateInstance() failed due to required extension not present (%d).\n", vkResult);
        return(vkResult);
    }
    else if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "createVulkanInstance() : vkCreateInstance() failed due to [unknown reason] (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "createVulkanInstance() : vkCreateInstance() succeeded.\n");
    }

    /*
     * sub-step 5 : destroy VkInstance in uninitialize()
     */
    
    // code in uninitialize()

    return(vkResult);
}

VkResult fillInstanceExtensionNames(void)
{
    // variable declarations
    VkResult vkResult = VK_SUCCESS;

    // code
    /* 
     * sub-step 1 : Find how many extensions are supported by the Vulkan 
     *              driver of this version and keep it in a local variable
     */
    uint32_t instanceExtensionCount = 0;
    
    vkResult = vkEnumerateInstanceExtensionProperties(
                                                      NULL,                    // [in, optional] layer name to retrieve extensions from (NULL means you want all extensions)
                                                      &instanceExtensionCount, // [out] count of supported extensions
                                                      NULL                     // [out, optional] array of VkExtensionProperties to retrieve extension properties
                                                     );     
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "fillInstanceExtensionNames() : first call to vkEnumerateInstanceExtensionProperties() failed.\n");
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "fillInstanceExtensionNames() : first call to vkEnumerateInstanceExtensionProperties() succeeded.\n");
    }

    /*
     * sub-step 2 : allocate and fill struct VkExtensionProperties array 
     *              corresponding to above count 
     */
    VkExtensionProperties *vkExtensionProperties_Array = NULL;
    vkExtensionProperties_Array                        = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * instanceExtensionCount);

    /*
     * for the sake of brevity, we are avoiding error checking for malloc()
     * but in real world, you should do this error-checking
     */

    vkResult = vkEnumerateInstanceExtensionProperties(
                                                      NULL,
                                                      &instanceExtensionCount,
                                                      vkExtensionProperties_Array
                                                     );
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "fillInstanceExtensionNames() : second call to vkEnumerateInstanceExtensionProperties() failed.\n");
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "fillInstanceExtensionNames() : second call to vkEnumerateInstanceExtensionProperties() succeeded.\n");
    }

    /*
     * sub-step 3 : fill & display a local string array of extension names 
     *              obtained from VkExtensionProperties struct array
     */
    char **instanceExtensionNames_Array = NULL;
    instanceExtensionNames_Array        = (char**)malloc(sizeof(char*) * instanceExtensionCount);

    fprintf(gpFILE, BORDER);

    for(uint32_t i = 0; i < instanceExtensionCount; i++)
    {
        instanceExtensionNames_Array[i] = (char*)malloc(sizeof(char) * (strlen(vkExtensionProperties_Array[i].extensionName) + 1));
        memcpy(
               instanceExtensionNames_Array[i],                         // destination
               vkExtensionProperties_Array[i].extensionName,            // source
               strlen(vkExtensionProperties_Array[i].extensionName) + 1 // length
              );
    
        fprintf(gpFILE, "fillInstanceExtensionNames() : Vulkan instance extension name = %s\n", instanceExtensionNames_Array[i]);
    }

    fprintf(gpFILE, BORDER);

    /*
     * sub-step 4 : as not required here onwards, free the VkExtensionProperties array
     */
    free(vkExtensionProperties_Array);
    vkExtensionProperties_Array = NULL;

    /*
     * sub-step 5 : find whether above extension names contain our required 2 extensions ->
     *                  (1) VK_KHR_SURFACE_EXTENSION_NAME
     *                  (2) VK_KHR_WIN32_SURFACE_EXTENSION_NAME
     *              
     *              Accordingly, set 2 global variables ->
     *                  (1) Required extension count
     *                  (2) Required extension names array
     */
    VkBool32 vulkanSurfaceExtensionFound = VK_FALSE;
    VkBool32 win32SurfaceExtensionFound  = VK_FALSE;

    for(uint32_t i = 0; i < instanceExtensionCount; i++)
    {
        // using macros is recommended, instead of actual extension names
        if(strcmp(instanceExtensionNames_Array[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0)
        {
            vulkanSurfaceExtensionFound                                          = VK_TRUE;
            enabledInstanceExtensionNames_Array[enabledInstanceExtensionCount++] = VK_KHR_SURFACE_EXTENSION_NAME; 
        }
        
        if(strcmp(instanceExtensionNames_Array[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0)
        {
            win32SurfaceExtensionFound                                           = VK_TRUE;
            enabledInstanceExtensionNames_Array[enabledInstanceExtensionCount++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME; 
        }
    } 

    /*
     * sub-step 6 : as not needed henceforth, free the local strings array
     */
    for(uint32_t i = 0; i < instanceExtensionCount; i++)
    {
        free(instanceExtensionNames_Array[i]);
        instanceExtensionNames_Array[i] = NULL;
    }

    free(instanceExtensionNames_Array);
    instanceExtensionNames_Array = NULL;

    /*
     * sub-step 7 : print whether our Vulkan driver 
     *              supports our required extensions or not
     */
    if(vulkanSurfaceExtensionFound == VK_FALSE)
    {
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // return hard-coded failure
        fprintf(gpFILE, "fillInstanceExtensionNames() : VK_KHR_SURFACE_EXTENSION_NAME not found.\n");
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "fillInstanceExtensionNames() : VK_KHR_SURFACE_EXTENSION_NAME found.\n");
    }

    if(win32SurfaceExtensionFound == VK_FALSE)
    {
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // return hard-coded failure
        fprintf(gpFILE, "fillInstanceExtensionNames() : VK_KHR_WIN32_SURFACE_EXTENSION_NAME not found.\n");
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "fillInstanceExtensionNames() : VK_KHR_WIN32_SURFACE_EXTENSION_NAME found.\n");
    }

    fprintf(gpFILE, BORDER);

    /*
     * sub-step 8 : print only enabled extension names
     */
    for(uint32_t i = 0; i < enabledInstanceExtensionCount; i++)
    {
        fprintf(gpFILE, "fillInstanceExtensionNames() : Enabled Vulkan instance extension name = %s\n", enabledInstanceExtensionNames_Array[i]);
    }

    fprintf(gpFILE, BORDER);

    return(vkResult);
}

VkResult getSupportedSurface(void)
{
    // variable declarations
    VkResult vkResult = VK_SUCCESS;

    // code
    /*
     * sub-step 1 : Declare and memset() a platform-specific 
     *              (Windows, Linux, Android, etc.) SurfaceCreateInfo structure.
     */
    VkWin32SurfaceCreateInfoKHR vkWin32SurfaceCreateInfoKHR;
    memset((void *)&vkWin32SurfaceCreateInfoKHR, 0, sizeof(VkWin32SurfaceCreateInfoKHR));

    /*
     * sub-step 2 : Initialize it, particularly its hinstance and hwnd members.
     */
    vkWin32SurfaceCreateInfoKHR.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    vkWin32SurfaceCreateInfoKHR.pNext     = NULL;
    vkWin32SurfaceCreateInfoKHR.flags     = 0;
    vkWin32SurfaceCreateInfoKHR.hinstance = (HINSTANCE)GetWindowLongPtr(ghwnd, GWLP_HINSTANCE); // this member can also be initialized by using "(HINSTANCE)GetModuleHandle(NULL);"
    vkWin32SurfaceCreateInfoKHR.hwnd      = ghwnd;

    /*
     * sub-step 3 : Now call vkCreateWin32SurfaceKHR() to create the presentation surface object.
     */
    vkResult = vkCreateWin32SurfaceKHR(
                                        vkInstance,                   // [in] Vulkan instance object (until you get device, Vulkan instance will be used)
                                        &vkWin32SurfaceCreateInfoKHR, // [in] Surface create info's address
                                        NULL,                         // [in] memory allocator
                                        &vkSurfaceKHR                 // [out] pointer to a VkSurfaceKHR object
                                      );

    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "getSupportedSurface() : vkCreateWin32SurfaceKHR() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "getSupportedSurface() : vkCreateWin32SurfaceKHR() succeeded.\n");
    }

    return(vkResult);
}

VkResult getPhysicalDevice(void)
{
    // variable declarations
    VkResult vkResult = VK_SUCCESS;

    // code
    /*
     * sub-step 2 : Call vkEnumeratePhysicalDevices() to get physical device count.
     */
    vkResult = vkEnumeratePhysicalDevices(
                                            vkInstance,           // [in] Vulkan instance handle
                                            &physicalDeviceCount, // [out] count of available physical devices
                                            NULL                  // [out, optional] VkPhysicalDevice array  
                                         );

    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "getPhysicalDevice() : 1st call to vkEnumeratePhysicalDevices() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else if(physicalDeviceCount == 0)
    {
        fprintf(gpFILE, "getPhysicalDevice() : 1st call to vkEnumeratePhysicalDevices() resulted in 0 devices.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // hard-coded result
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "getPhysicalDevice() : 1st call to vkEnumeratePhysicalDevices() succeeded.\n");
    }

    /*
     * sub-step 3 : Allocate VkPhysicalDevice array according to above count.
     */
    vkPhysicalDevice_Array = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);

    /*
     * sub-step 4 : Call vkEnumeratePhysicalDevices() again to fill the above array.
     */
    vkResult = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, vkPhysicalDevice_Array);

    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "getPhysicalDevice() : 2nd call to vkEnumeratePhysicalDevices() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "getPhysicalDevice() : 2nd call to vkEnumeratePhysicalDevices() succeeded.\n");
    }

    /*
     * sub-step 5 : Start a loop using the above physical device count and physical device array.
     */
    VkBool32 bFound = VK_FALSE;

    for(uint32_t i = 0; i < physicalDeviceCount; i++)
    {
        /*
         * sub-sub-step (1) : Declare a local variable to hold queue count
         */
        uint32_t queueCount = UINT32_MAX;

        /*
         * sub-sub-step (2) : Call vkGetPhysicalDeviceQueueFamilyProperties() to 
         *                    initialize the above queue count variable.
         */
        vkGetPhysicalDeviceQueueFamilyProperties(
                                                 vkPhysicalDevice_Array[i], // [in] Vulkan physical device
                                                 &queueCount,               // [out] Queue family count
                                                 NULL                       // [out, optional] VkQueueFamilyProperties array 
                                                );

        /*
         * sub-sub-step (3) : Allocate VkQueueFamilyProperties array according to above count.
         */
        VkQueueFamilyProperties *vkQueueFamilyProperties_Array = NULL;
        vkQueueFamilyProperties_Array = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueCount);

        /*
         * sub-sub-step (4) : Call vkGetPhysicalDeviceQueueFamilyProperties() again to fill the above array.
         */
        vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice_Array[i], &queueCount, vkQueueFamilyProperties_Array);

        /*
         * sub-sub-step (5) : Declare a VkBool32 type array and allocate it using the same above queue count.
         */
        VkBool32 *isQueueSurfaceSupported_Array = NULL;
        isQueueSurfaceSupported_Array = (VkBool32*)malloc(sizeof(VkBool32) * queueCount);

        /*
         * sub-sub-step (6) : Start a nested loop and fill above VkBool32 type array by 
         *                    calling vkGetPhysicalDeviceSurfaceSupportKHR().
         */
        for(uint32_t j = 0; j < queueCount; j++)
        {
            vkGetPhysicalDeviceSurfaceSupportKHR(
                                                 vkPhysicalDevice_Array[i],        // [in] Vulkan physical device
                                                 j,                                // [in] Queue family index
                                                 vkSurfaceKHR,                     // [in] VkSurfaceKHR object
                                                 &isQueueSurfaceSupported_Array[j] // [out] is the queue family supported by the surface?
                                                );
        }

        /*
         * sub-sub-step (7) : Start another nested loop (not nested in above loop, but nested in main loop) and 
         *                    check whether the physical device in its array with its queue family has the graphics bit or not. 
         *                    If yes, then this is a selected physical device so assign it to the global variable. 
         *                    Similarly, if this index is the selected queue family index, assign it to the global variable too. 
         *                    Set bFound = VK_TRUE and break out from the 2nd nested loop.
         */
        for(uint32_t j = 0; j < queueCount; j++)
        {
            if(vkQueueFamilyProperties_Array[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) // there are also COMPUTE / TRANSFER bits which can be checked
            {
                if(isQueueSurfaceSupported_Array[j] == VK_TRUE)
                {
                    vkPhysicalDevice_Selected         = vkPhysicalDevice_Array[i];
                    graphicsQueueFamilyIndex_Selected = j;
                    bFound                            = VK_TRUE;

                    break;
                }
            }
        }

        /*
         * sub-sub-step (8) : Now we are back in the main loop, 
         *                    so free the 2 arrays -> Queue family array and the VkBool32 array.
         */
        if(isQueueSurfaceSupported_Array)
        {
            free(isQueueSurfaceSupported_Array);
            isQueueSurfaceSupported_Array = NULL;

            fprintf(gpFILE, "getPhysicalDevice() : succeeded to free isQueueSurfaceSupported_Array.\n");
        }

        if(vkQueueFamilyProperties_Array)
        {
            free(vkQueueFamilyProperties_Array);
            vkQueueFamilyProperties_Array = NULL;

            fprintf(gpFILE, "getPhysicalDevice() : succeeded to free vkQueueFamilyProperties_Array.\n");
        }

        /*
         * sub-sub-step (9) : Still being in the main loop, according to the bFound variable, break out from the main loop.
         */
        if(bFound == VK_TRUE)
        {
            break;
        }
    }

    /*
     * sub-step 6 : Do error checking according to the value of bFound.
     */
    if(bFound == VK_TRUE)
    {
        fprintf(gpFILE, "getPhysicalDevice() : succeeded to select required physical device with graphics enabled.\n");
    }
    else
    {
        if(vkPhysicalDevice_Array)
        {
            free(vkPhysicalDevice_Array);
            vkPhysicalDevice_Array = NULL;

            fprintf(gpFILE, "getPhysicalDevice() : succeeded to free vkPhysicalDevice_Array.\n");
        }

        fprintf(gpFILE, "getPhysicalDevice() : failed to select graphics supported physical device.\n");
        
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return(vkResult);
    }

    /*
     * sub-step 7 : memset() the global physical device memory property structure
     */
    memset((void *)&vkPhysicalDeviceMemoryProperties, 0, sizeof(VkPhysicalDeviceMemoryProperties));

    /*
     * sub-step 8 : Initialize above structure by using vkGetPhysicalDeviceMemoryProperties(). 
     */
    vkGetPhysicalDeviceMemoryProperties(
                                        vkPhysicalDevice_Selected,        // [in] Vulkan physical device
                                        &vkPhysicalDeviceMemoryProperties // [out] address to a structure of VkPhysicalDeviceMemoryProperties 
                                       );

    /*
     * sub-step 9 : Declare a local structure variable VkPhysicalDeviceFeatures, memset() it and 
     *              initialize it by calling vkGetPhysicalDeviceFeatures().
     */
    VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures;
    memset((void *)&vkPhysicalDeviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));

    vkGetPhysicalDeviceFeatures(
                                vkPhysicalDevice_Selected, // [in] Vulkan physical device
                                &vkPhysicalDeviceFeatures  // [out] address to a structure of VkPhysicalDeviceFeatures
                               );

    /*
     * sub-step 10 : By using the “tessellationShader” member of the above structure, 
     *               check the selected device’s tessellation shader support.
     */
    if(vkPhysicalDeviceFeatures.tessellationShader == VK_TRUE)
    {
        fprintf(gpFILE, "getPhysicalDevice() : Selected physical device supports tessellation shader.\n");
    }
    else
    {
        fprintf(gpFILE, "getPhysicalDevice() : Selected physical device doesn't support tessellation shader.\n");
    }

    /*
     * sub-step 11 : By using the “geometryShader” member of the above structure, 
     *               check the selected device’s geometry shader support.
     */
    if(vkPhysicalDeviceFeatures.geometryShader == VK_TRUE)
    {
        fprintf(gpFILE, "getPhysicalDevice() : Selected physical device supports geometry shader.\n");
    }
    else
    {
        fprintf(gpFILE, "getPhysicalDevice() : Selected physical device doesn't support geometry shader.\n");
    }

    return(vkResult);
}

VkResult printVkInfo(void)
{
    // local variables
    VkResult vkResult = VK_SUCCESS;

    // code
    /*
     * sub-step (3) : Write printVkInfo() user-defined function
     */
    fprintf(gpFILE, BORDER);
    fprintf(gpFILE, "*                                        Vulkan Information                                        *\n");
    fprintf(gpFILE, BORDER);

    /*
     * step (a) : Start a loop using global physical device count 
     *            and inside it declare and memset() VkPhysicalDeviceProperties struct variable.
     */
    for(uint32_t i = 0; i < physicalDeviceCount; i++)
    {
        VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
        memset((void *)&vkPhysicalDeviceProperties, 0, sizeof(VkPhysicalDeviceProperties));

        /*
         * step (b) : Initialize this struct variable by calling vkGetPhysicalDeviceProperties() Vulkan API
         */
        vkGetPhysicalDeviceProperties(
                                        vkPhysicalDevice_Array[i],  // [in] VkPhysicalDevice
                                        &vkPhysicalDeviceProperties // [out] VkPhysicalDeviceProperties
                                     );

        /*
         * step (c) : Print Vulkan API version using “apiVersion” member of above struct. This requires 3 Vulkan macros.
         */
        uint32_t majorVersion = VK_VERSION_MAJOR(vkPhysicalDeviceProperties.apiVersion);
        uint32_t minorVersion = VK_API_VERSION_MINOR(vkPhysicalDeviceProperties.apiVersion);
        uint32_t patchVersion = VK_API_VERSION_PATCH(vkPhysicalDeviceProperties.apiVersion);

        fprintf(gpFILE, "printVkInfo() : API Version = %d.%d.%d\n", majorVersion, minorVersion, patchVersion);
    
        /*
         * step (d) : Print device name by using “deviceName” member of above struct.
         */
        fprintf(gpFILE, "printVkInfo() : Device Name = %s\n", vkPhysicalDeviceProperties.deviceName);
    
        /*
         * step (e) : Use the “deviceType” member of the above struct in a switch case block 
         *            and accordingly print device type.
         */
        switch(vkPhysicalDeviceProperties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            fprintf(gpFILE, "printVkInfo() : Device Type = Integrated GPU (iGPU)\n");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            fprintf(gpFILE, "printVkInfo() : Device Type = Discrete GPU (dGPU)\n");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            fprintf(gpFILE, "printVkInfo() : Device Type = Virtual GPU (vGPU)\n");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            fprintf(gpFILE, "printVkInfo() : Device Type = CPU\n");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            fprintf(gpFILE, "printVkInfo() : Device Type = Other\n");
            break;
        default:
            fprintf(gpFILE, "printVkInfo() : Device Type = UNKNOWN\n");
            break;
        }

        /*
         * step (f) : Print hexadecimal vendor ID of the device using the “vendorID” member of the above struct.
         */
        fprintf(gpFILE, "printVkInfo() : Vendor ID   = 0x%04x\n", vkPhysicalDeviceProperties.vendorID);

        /*
         * step (g) : Print hexadecimal device ID using the “deviceID” member of the above struct. 
         *
         * [Note : for the sake of completeness, 
         * we can repeat step (5) –-> (a) to (h) from getPhysicalDevice() 
         * but now instead of assigning selected queue and selected device, 
         * print whether this device supports Graphics Bit, Compute Bit, Transfer Bit using if else-if blocks. 
         * Similarly, we also can repeat device features from getPhysicalDevice() and can print all, 
         * around 50+ device features including support for tessellation shader and geometry shader.]
         */
        fprintf(gpFILE, "printVkInfo() : Device ID   = 0x%04x\n", vkPhysicalDeviceProperties.deviceID);
    }

    fprintf(gpFILE, BORDER);

    /*
     * step (h) : Free physical device array here, which we removed from the if(bFound == VK_TRUE) block of getPhysicalDevice().
     */
    if(vkPhysicalDevice_Array)
    {
        free(vkPhysicalDevice_Array);
        vkPhysicalDevice_Array = NULL;

        fprintf(gpFILE, "printVkInfo() : succeeded to free vkPhysicalDevice_Array.\n");
    }

    return(vkResult);
}

VkResult fillDeviceExtensionNames(void)
{
    // variable declarations
    VkResult vkResult = VK_SUCCESS;

    // code
    /* 
     * sub-step 1 : Find how many extensions are supported by the Vulkan 
     *              driver of this version and keep it in a local variable
     */
    uint32_t deviceExtensionCount = 0;
    
    vkResult = vkEnumerateDeviceExtensionProperties(
                                                    vkPhysicalDevice_Selected, // [in] VkPhysicalDevice 
                                                    NULL,                      // [in, optional] layer name to retrieve extensions from (NULL means you want all extensions)
                                                    &deviceExtensionCount,     // [out] count of supported extensions
                                                    NULL                       // [out, optional] array of VkExtensionProperties to retrieve extension properties
                                                   );     
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "fillDeviceExtensionNames() : first call to vkEnumerateDeviceExtensionProperties() failed.\n");
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "fillDeviceExtensionNames() : first call to vkEnumerateDeviceExtensionProperties() succeeded.\n");
    }

    /*
     * sub-step 2 : allocate and fill struct VkExtensionProperties array 
     *              corresponding to above count 
     */
    VkExtensionProperties *vkExtensionProperties_Array = NULL;
    vkExtensionProperties_Array                        = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * deviceExtensionCount);

    /*
     * for the sake of brevity, we are avoiding error checking for malloc()
     * but in real world, you should do this error-checking
     */

    vkResult = vkEnumerateDeviceExtensionProperties(
                                                    vkPhysicalDevice_Selected,
                                                    NULL,
                                                    &deviceExtensionCount,
                                                    vkExtensionProperties_Array
                                                   );
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "fillDeviceExtensionNames() : second call to vkEnumerateDeviceExtensionProperties() failed.\n");
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "fillDeviceExtensionNames() : second call to vkEnumerateDeviceExtensionProperties() succeeded.\n");
    }

    /*
     * sub-step 3 : fill & display a local string array of extension names 
     *              obtained from VkExtensionProperties struct array
     */
    char **deviceExtensionNames_Array = NULL;
    deviceExtensionNames_Array        = (char**)malloc(sizeof(char*) * deviceExtensionCount);

    fprintf(gpFILE, BORDER);

    for(uint32_t i = 0; i < deviceExtensionCount; i++)
    {
        deviceExtensionNames_Array[i] = (char*)malloc(sizeof(char) * (strlen(vkExtensionProperties_Array[i].extensionName) + 1));
        memcpy(
               deviceExtensionNames_Array[i],                           // destination
               vkExtensionProperties_Array[i].extensionName,            // source
               strlen(vkExtensionProperties_Array[i].extensionName) + 1 // length
              );
    
        fprintf(gpFILE, "fillDeviceExtensionNames() : Vulkan device extension name = %s\n", deviceExtensionNames_Array[i]);
    }

    fprintf(gpFILE, BORDER);

    /*
     * sub-step 4 : as not required here onwards, free the VkExtensionProperties array
     */
    free(vkExtensionProperties_Array);
    vkExtensionProperties_Array = NULL;

    /*
     * sub-step 5 : find whether above extension names contain our required 1 extension ->
     *                  (1) VK_KHR_SWAPCHAIN_EXTENSION_NAME
     *              
     *              Accordingly, set 2 global variables ->
     *                  (1) Required extension count
     *                  (2) Required extension names array
     */
    VkBool32 vulkanSwapchainExtensionFound = VK_FALSE;

    for(uint32_t i = 0; i < deviceExtensionCount; i++)
    {
        // using macros is recommended, instead of actual extension names
        if(strcmp(deviceExtensionNames_Array[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
        {
            vulkanSwapchainExtensionFound                                    = VK_TRUE;
            enabledDeviceExtensionNames_Array[enabledDeviceExtensionCount++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME; 
        }
    } 

    /*
     * sub-step 6 : as not needed henceforth, free the local strings array
     */
    for(uint32_t i = 0; i < deviceExtensionCount; i++)
    {
        free(deviceExtensionNames_Array[i]);
        deviceExtensionNames_Array[i] = NULL;
    }

    free(deviceExtensionNames_Array);
    deviceExtensionNames_Array = NULL;

    /*
     * sub-step 7 : print whether our Vulkan driver 
     *              supports our required extensions or not
     */
    if(vulkanSwapchainExtensionFound == VK_FALSE)
    {
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // return hard-coded failure
        fprintf(gpFILE, "fillDeviceExtensionNames() : VK_KHR_SWAPCHAIN_EXTENSION_NAME not found.\n");
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "fillDeviceExtensionNames() : VK_KHR_SWAPCHAIN_EXTENSION_NAME found.\n");
    }

    fprintf(gpFILE, BORDER);

    /*
     * sub-step 8 : print only enabled extension names
     */
    for(uint32_t i = 0; i < enabledDeviceExtensionCount; i++)
    {
        fprintf(gpFILE, "fillDeviceExtensionNames() : Enabled Vulkan device extension name = %s\n", enabledDeviceExtensionNames_Array[i]);
    }

    fprintf(gpFILE, BORDER);

    return(vkResult);
}

/*
 * sub-step 1 : Create a user-defined function “createVulkanDevice()”.
 */
VkResult createVulkanDevice(void)
{
    // function declarations
    VkResult fillDeviceExtensionNames(void);

    // variable declarations
    VkResult vkResult = VK_SUCCESS;

    // code
    /*
     * sub-step 2 : Call previously created fillDeviceExtensionNames() in it.
     */

    // STEP 7 : fill and initialize required device extension names and count global variables
    vkResult = fillDeviceExtensionNames();
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "createVulkanDevice() : fillDeviceExtensionNames() failed.\n");
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "createVulkanDevice() : fillDeviceExtensionNames() succeeded.\n");
    }

    /*
     * sub-step 3 : Declare and initialize VkDeviceCreateInfo structure. 
     *              Use previously obtained device extension count and 
     *              device extension array to initialize this structure.
     */
    // newly added code (after vkGetDeviceQueue() was returning VK_NULL_HANDLE)
    VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo;
    memset((void *)&vkDeviceQueueCreateInfo, 0, sizeof(VkDeviceQueueCreateInfo));
 
    float queuePriorities[1];
    queuePriorities[0] = 0.0f;

    vkDeviceQueueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    vkDeviceQueueCreateInfo.pNext            = NULL;
    vkDeviceQueueCreateInfo.flags            = 0;
    vkDeviceQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex_Selected;
    vkDeviceQueueCreateInfo.queueCount       = 1;
    vkDeviceQueueCreateInfo.pQueuePriorities = queuePriorities; // default queue priority
 
    VkDeviceCreateInfo vkDeviceCreateInfo;
    memset((void *)&vkDeviceCreateInfo, 0, sizeof(VkDeviceCreateInfo));

    vkDeviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vkDeviceCreateInfo.pNext                   = NULL;
    vkDeviceCreateInfo.flags                   = 0;
    vkDeviceCreateInfo.enabledExtensionCount   = enabledDeviceExtensionCount;
    vkDeviceCreateInfo.ppEnabledExtensionNames = enabledDeviceExtensionNames_Array;
    vkDeviceCreateInfo.enabledLayerCount       = 0;
    vkDeviceCreateInfo.ppEnabledLayerNames     = NULL;
    vkDeviceCreateInfo.pEnabledFeatures        = NULL;

    // newly added code (after vkGetDeviceQueue() was returning VK_NULL_HANDLE)
    vkDeviceCreateInfo.queueCreateInfoCount    = 1;
    vkDeviceCreateInfo.pQueueCreateInfos       = &vkDeviceQueueCreateInfo;

    /*
     * sub-step 4 : Now call vkCreateDevice() Vulkan API to actually 
     *              create the Vulkan device and do error-checking.
     */
    vkResult = vkCreateDevice(
                              vkPhysicalDevice_Selected, // [in] Vulkan physical device handle
                              &vkDeviceCreateInfo,       // [in] VkDeviceCreateInfo*
                              NULL,                      // [in, optional] pointer to a custom memory allocator
                              &vkDevice                  // [out] VkDevice*
                             );

    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "createVulkanDevice() : vkCreateDevice() failed.\n");
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "createVulkanDevice() : vkCreateDevice() succeeded.\n");
    }

    return(vkResult);
}

void getDeviceQueue(void)
{
    // code
    /*
     * sub-step 1 : Call vkGetDeviceQueue() using newly created VkDevice, 
     *              selected family index, 0th queue in that selected queue family.
     */
    vkGetDeviceQueue(
                     vkDevice,                          // [in] vulkan logical device handle 
                     graphicsQueueFamilyIndex_Selected, // [in] selected queue family index
                     0,                                 // [in] queue family index
                     &vkQueue                           // [out] VkQueue* 
                    );

    if(vkQueue == VK_NULL_HANDLE)
    {
        fprintf(gpFILE, "getDeviceQueue() : vkGetDeviceQueue() returned NULL for VkQueue.\n");
        return ;
    }
    else
    {
        fprintf(gpFILE, "getDeviceQueue() : vkGetDeviceQueue() succeeded.\n");
    }
}

VkResult getPhysicalDeviceSurfaceFormatAndColorSpace(void)
{
    // Variable declaration
    VkResult vkResult = VK_SUCCESS;

    // code
    // get the count of supported color surface format

    uint32_t formatCount = 0;

    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice_Selected, vkSurfaceKHR, &formatCount, NULL);

    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "1st call getPhysicalDeviceSurfaceFormatAndColorSpace() : vkGetPhysicalDeviceSurfaceFormatsKHR() failed.\n");
        return(vkResult);
    }
    else if(formatCount == 0)
    {
        fprintf(gpFILE, "1st Call getPhysicalDeviceSurfaceFormatAndColorSpace() : vkGetPhysicalDeviceSurfaceFormatsKHR() result is 0.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // hard-coded result
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "1st Call getPhysicalDeviceSurfaceFormatAndColorSpace() : vkGetPhysicalDeviceSurfaceFormatsKHR() succeeded.\n");
    }

    // declare and allocate vkSurfaceFormatKHR_Array
    VkSurfaceFormatKHR* vkSurfaceFormatKHR_Array = (VkSurfaceFormatKHR*)malloc(formatCount * sizeof(VkSurfaceFormatKHR));

    // Filling the array
    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice_Selected, vkSurfaceKHR, &formatCount, vkSurfaceFormatKHR_Array);

    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "2nd Call getPhysicalDeviceSurfaceFormatAndColorSpace() : vkGetPhysicalDeviceSurfaceFormatsKHR() failed.\n");
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "2nd call getPhysicalDeviceSurfaceFormatAndColorSpace() : vkGetPhysicalDeviceSurfaceFormatsKHR() succeeded formatCount = %d.\n", formatCount);
    }

    // Decide the color format first and then color space
    if(formatCount == 1 && vkSurfaceFormatKHR_Array[0].format == VK_FORMAT_UNDEFINED)
    {
        vkFormat_color = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        vkFormat_color = vkSurfaceFormatKHR_Array[0].format;
    }

    // Decide the color space 
    vkColorSpaceKHR = vkSurfaceFormatKHR_Array[0].colorSpace;

    if(vkSurfaceFormatKHR_Array)
    {
        free(vkSurfaceFormatKHR_Array);
        vkSurfaceFormatKHR_Array = NULL;
        fprintf(gpFILE, "vkSurfaceFormatKHR_Array formated.\n");
    }

    return vkResult;
}


VkResult getPhysicalDevicePresentMode(void)
{
    // Variable declaration
    VkResult vkResult = VK_SUCCESS;

    // code
    uint32_t presentModeCount = 0;

    vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice_Selected, vkSurfaceKHR, &presentModeCount, NULL);
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "1st call getPhysicalDevicePresentMode() : VkGetPhysicalDeviceSurfacePresentModesKHR() failed.\n");
        return(vkResult);
    }
    else if(presentModeCount == 0)
    {
        fprintf(gpFILE, "1st Call getPhysicalDevicePresentMode() : VkGetPhysicalDeviceSurfacePresentModesKHR() result is 0.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // hard-coded result
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "1st Call getPhysicalDevicePresentMode() : VkGetPhysicalDeviceSurfacePresentModesKHR() succeeded. presentModeCount = %d\n", presentModeCount);
    }

    VkPresentModeKHR* vkPresentMode_Array = (VkPresentModeKHR*)malloc(presentModeCount * sizeof(VkPresentModeKHR));

    vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice_Selected, vkSurfaceKHR, &presentModeCount, vkPresentMode_Array);
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "2nd call getPhysicalDevicePresentMode() : VkGetPhysicalDeviceSurfacePresentModesKHR() failed.\n");
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "2nd Call getPhysicalDevicePresentMode() : VkGetPhysicalDeviceSurfacePresentModesKHR() succeeded.\n");
    }

    // Deside Presentation mode
    for(uint32_t i = 0; i<presentModeCount; i++)
    {
        fprintf(gpFILE, "** %d **\n", vkPresentMode_Array[i]);
        if(vkPresentMode_Array[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            vkPresentModeKHR = VK_PRESENT_MODE_MAILBOX_KHR;
           // break;
        }
    }

    if(vkPresentModeKHR != VK_PRESENT_MODE_MAILBOX_KHR)
    {
        vkPresentModeKHR = VK_PRESENT_MODE_FIFO_KHR;
    }

    if(vkPresentMode_Array)
    {
        free(vkPresentMode_Array);
        vkPresentMode_Array = NULL;
        fprintf(gpFILE, "vkPresentMode_Array formated.\n");
    }

    return vkResult;
}

VkResult createSwapchain(VkBool32 vSync) // tatency chalnar nasel tr false (Presentation mode wr adjuct krave), and latency chalnar asel tr true.
{
    // Function Declarations
    VkResult getPhysicalDeviceSurfaceFormatAndColorSpace(void);
    VkResult getPhysicalDevicePresentMode(void);

    // variable declaration 
    VkResult vkResult = VK_SUCCESS;

    // code 

    /*
        Step 1 : get physical device surface supported color format and physical device supported colorspace
     */

    // Get Color Format and Color Space
    vkResult = getPhysicalDeviceSurfaceFormatAndColorSpace();

    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "createSwapchain() : getPhysicalDeviceSurfaceFormatAndColorSpace() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "createSwapchain() : getPhysicalDeviceSurfaceFormatAndColorSpace() succeeded.\n");
    }




    /*
    *
        Step 2 : get physical device capabilities by using vulkan api vkGetPhysicalDeviceSurfaceCapabilitiesKHR() and accordingly initialize VkSurfaceCapabilitiesKHR structure
     *
    */
    VkSurfaceCapabilitiesKHR vkSurefaceCapabilitiesKHR;
    memset((void*)&vkSurefaceCapabilitiesKHR, 0,sizeof(VkSurfaceCapabilitiesKHR));

    vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                                                        vkPhysicalDevice_Selected,
                                                        vkSurfaceKHR,
                                                        &vkSurefaceCapabilitiesKHR
                                                    );

    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "createSwapchain() : vkGetPhysicalDeviceSurfaceCapabilitiesKHR() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "createSwapchain() : vkGetPhysicalDeviceSurfaceCapabilitiesKHR() succeeded.\n");
    }





    /*
    *
        Step 3 : by using minImageCount and maxImageCount from above structure decide desire image count for Swapchain
    *
    */
    uint32_t testingNumberOfSwapchainImages = vkSurefaceCapabilitiesKHR.minImageCount + 1;
    uint32_t desiredSwapchainImages = 0;

    fprintf(gpFILE, "createSwapchain() : MinImageCount : %d  MaxImageCount : %d.\n", vkSurefaceCapabilitiesKHR.minImageCount ,vkSurefaceCapabilitiesKHR.maxImageCount );
    if(vkSurefaceCapabilitiesKHR.maxImageCount > 0 && vkSurefaceCapabilitiesKHR.maxImageCount < testingNumberOfSwapchainImages)
    {
        desiredSwapchainImages = vkSurefaceCapabilitiesKHR.maxImageCount;
    }
    else 
    {
        desiredSwapchainImages = vkSurefaceCapabilitiesKHR.minImageCount;
    }





    /*
    *
        Step 4 : by using currentExtent.width and currentExtent.height members of above structure and comparing them with current width and height of window decide image width and image height of swapchain.
    *
    */
    memset((void*)&vkExtent2D_Swapchain, 0, sizeof(VkExtent2D));
    if(vkSurefaceCapabilitiesKHR.currentExtent.width != UINT32_MAX)
    {
        vkExtent2D_Swapchain.width = vkSurefaceCapabilitiesKHR.currentExtent.width;
        vkExtent2D_Swapchain.height = vkSurefaceCapabilitiesKHR.currentExtent.height;

        fprintf(gpFILE, "createSwapchain() : 1st SwapchainImage Width * height : %d * %d\n", vkExtent2D_Swapchain.width, vkExtent2D_Swapchain.height);
    }
    else
    {
        // if surface size is already defined them swapchain size must match with it
        VkExtent2D vkExtent2D;
        memset((void*)&vkExtent2D, 0, sizeof(VkExtent2D));
        vkExtent2D.width = (uint32_t)winWidth;
        vkExtent2D.height = (uint32_t)winHeight;

        vkExtent2D_Swapchain.width = max(vkSurefaceCapabilitiesKHR.minImageExtent.width, min(vkSurefaceCapabilitiesKHR.maxImageExtent.width, vkExtent2D.width));
        vkExtent2D_Swapchain.height = max(vkSurefaceCapabilitiesKHR.minImageExtent.height, min(vkSurefaceCapabilitiesKHR.maxImageExtent.height, vkExtent2D.height));

        fprintf(gpFILE, "createSwapchain() : 2nd SwapchainImage Width * height : %d * %d\n", vkExtent2D_Swapchain.width, vkExtent2D_Swapchain.height);
    }



    
    /*
    *
        Step 5 : Decide how we are going to use swapchain images means whether we are going to store image data and use it later (Differed rendering) or whether we are going to use it as color attachment
    *
    */
    VkImageUsageFlags vkImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT    // This Macro is needed for color attachment we are not currently using differed shading here
                                        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;      // This Macro is not usefull here triangle like application but its neede when we working on FBO, compute, texture




    /*
    *
        Step 6 : While creating Swapchain we can decide whether to pretransform or not , pretransform also includes flipping image.
    *
    */

    VkSurfaceTransformFlagBitsKHR vkSurfaceTransformFlagBitsKHR; // ENUM type
    
    if(vkSurefaceCapabilitiesKHR.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        vkSurfaceTransformFlagBitsKHR = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        vkSurfaceTransformFlagBitsKHR = vkSurefaceCapabilitiesKHR.currentTransform;
    }

    /*
    *
        Step 7 : get presentation mode for swapchain images.
    *
    */
    // Presentation Mode
    vkResult = getPhysicalDevicePresentMode();

    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "createSwapchain() : getPhysicalDevicePresentMode() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "createSwapchain() : getPhysicalDevicePresentMode() succeeded.\n");
    }




    /*
    *
        Step 8 : according to above data declare, memset and initialize VkSwapchainCreateInfo structure.
    *
    */

    // initialize VkSwapchainCreateInfo structure
    VkSwapchainCreateInfoKHR vkSwapchainCreatInfoKHR;
    memset((void*)&vkSwapchainCreatInfoKHR, 0, sizeof(VkSwapchainCreateInfoKHR));

    vkSwapchainCreatInfoKHR.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vkSwapchainCreatInfoKHR.pNext = NULL;
    vkSwapchainCreatInfoKHR.flags = 0;
    vkSwapchainCreatInfoKHR.surface = vkSurfaceKHR;
    vkSwapchainCreatInfoKHR.minImageCount = desiredSwapchainImages;
    vkSwapchainCreatInfoKHR.imageFormat = vkFormat_color;
    vkSwapchainCreatInfoKHR.imageColorSpace = vkColorSpaceKHR;
    vkSwapchainCreatInfoKHR.imageExtent.width = vkExtent2D_Swapchain.width;
    vkSwapchainCreatInfoKHR.imageExtent.height = vkExtent2D_Swapchain.height;
    vkSwapchainCreatInfoKHR.imageUsage = vkImageUsageFlags;
    vkSwapchainCreatInfoKHR.preTransform = vkSurfaceTransformFlagBitsKHR;
    vkSwapchainCreatInfoKHR.imageArrayLayers = 1;
    vkSwapchainCreatInfoKHR.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkSwapchainCreatInfoKHR.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    vkSwapchainCreatInfoKHR.presentMode = vkPresentModeKHR;
    vkSwapchainCreatInfoKHR.clipped = VK_TRUE;



    
    /*
    *
        Step 9 : At the end call vkCreateSwapchainKHR() vulkan api to create the swapchain.

    *
    */

    vkResult = vkCreateSwapchainKHR(vkDevice, &vkSwapchainCreatInfoKHR, NULL, &vkSwapchainKHR);
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "createSwapchain() : VkCreateSwapchainKHR() failed (%d).\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "createSwapchain() : VkCreateSwapchainKHR() succeeded.\n");
    }

    return(vkResult);
}


VkResult CreateImagesAndImageViews(void)
{
    // Variable Declaraions
    VkResult vkResult = VK_SUCCESS;


    /*
    *
        Step 1 : Get swapchain image count in global variable using VkGetSwapchainImagesKHR()

    *
    */

    vkResult = vkGetSwapchainImagesKHR(vkDevice, vkSwapchainKHR ,&swapchainImageCount, NULL);
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "1st call CreateImagesAndImageViews() : vkGetSwapchainImagesKHR() failed.\n");
        return(vkResult);
    }
    else if(swapchainImageCount == 0)
    {
        fprintf(gpFILE, "1st Call CreateImagesAndImageViews() : vkGetSwapchainImagesKHR() result is 0.\n");
        vkResult = VK_ERROR_INITIALIZATION_FAILED; // hard-coded result
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "CreateImagesAndImageViews() : vkGetSwapchainImagesKHR() gave SwapchainImageCount = %d.\n", swapchainImageCount);
    }




    /*
    *
        Step 2 : Declare a global VkImageType Array and allocate it to the swapchain image count using malloc()

    *
    */
   swapchainImage_Array = (VkImage *)malloc(sizeof(VkImage) * swapchainImageCount);



   /*
    *
       Step 3 : Now call the same fuction again which we called in step 1 and fill this Array

    *
    */

    vkResult = vkGetSwapchainImagesKHR(vkDevice, vkSwapchainKHR, &swapchainImageCount, swapchainImage_Array);
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "2nd call CreateImagesAndImageViews() : vkGetSwapchainImagesKHR() failed.\n");
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "2nd call CreateImagesAndImageViews() : vkGetSwapchainImagesKHR() gave SwapchainImageCount = %d.\n", swapchainImageCount);
    }


    /*
    *
       Step 4 : Declare another global array of type VkImageView and Allocate it to the size of swapchain image count

    *
    */
    swapchainImageView_Array = (VkImageView *)malloc(sizeof(VkImageView) * swapchainImageCount);


    /*
    *
       Step 5 : Declare and initialize VkImageViewCreateInfo structure exept it ".image" member

    *
    */
   VkImageViewCreateInfo vkImageViewCreateInfo;

   memset((void*)&vkImageViewCreateInfo, 0,sizeof(VkImageViewCreateInfo));

   vkImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   vkImageViewCreateInfo.pNext = NULL;
   vkImageViewCreateInfo.flags = 0;
   vkImageViewCreateInfo.format = vkFormat_color;
   vkImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
   vkImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
   vkImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
   vkImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
   vkImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   vkImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
   vkImageViewCreateInfo.subresourceRange.levelCount = 1;
   vkImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
   vkImageViewCreateInfo.subresourceRange.layerCount = 1;
   vkImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;


    /*
    *
       Step 6 : Now start a loop for swapchain image count and inside this loop initialize above ".image" member to the swapchain image aaray index we obtained above And then call
                 VkCreateImageView Api to fill above ImageView Array
    *
    */
   for(uint32_t i = 0; i < swapchainImageCount; i++)
   {
        vkImageViewCreateInfo.image = swapchainImage_Array[i];

        vkResult = vkCreateImageView(vkDevice, &vkImageViewCreateInfo, NULL, &swapchainImageView_Array[i]);
        if(vkResult != VK_SUCCESS)
        {
            fprintf(gpFILE, "call CreateImagesAndImageViews() : vkCreateImageViews() failed for iteration %d. (%d)\n", i, vkResult);
            return(vkResult);
        }
        else
        {
            fprintf(gpFILE, "CreateImagesAndImageViews() : vkCreateImageViews() succeeded for iteration %d\n",i);
        }
   }
   

    return(vkResult);
}


VkResult CreateCommandPool(void)
{
     // Variable Declaraions
     VkResult vkResult = VK_SUCCESS;

    // code 
    /*
    *
       Step 1 : Declare and initialize vkCommadPoolCreateInfoStructure.
    *
    */
   VkCommandPoolCreateInfo  vkCommandPoolCreateInfo;

   memset((void*)&vkCommandPoolCreateInfo, 0, sizeof(VkCommandPoolCreateInfo));

   vkCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   vkCommandPoolCreateInfo.pNext = NULL;
   vkCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
   vkCommandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex_Selected;

   vkResult = vkCreateCommandPool(vkDevice, &vkCommandPoolCreateInfo, NULL, &vkCommandPool);
   if(vkResult != VK_SUCCESS)
   {
       fprintf(gpFILE, "CreateCommandPool() : vkCreateCommandPool() failed %d.\n", vkResult);
       return(vkResult);
   }
   else
   {
       fprintf(gpFILE, "CreateCommandPool() : vkCreateCommandPool() succeeded \n");
   }

     return (vkResult);
}


VkResult CreateCommandBuffers(void)
{
     // Variable Declaraions
     VkResult vkResult = VK_SUCCESS;

    // code 
    VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo;

    memset((void *)&vkCommandBufferAllocateInfo, 0, sizeof(VkCommandBufferAllocateInfo));

    vkCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vkCommandBufferAllocateInfo.pNext = NULL;
    vkCommandBufferAllocateInfo.commandPool = vkCommandPool;
    vkCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkCommandBufferAllocateInfo.commandBufferCount = 1;


    vkCommandBuffer_Array = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * swapchainImageCount);


    for(uint32_t i = 0; i<swapchainImageCount; i++)
    {
        vkResult = vkAllocateCommandBuffers(vkDevice, &vkCommandBufferAllocateInfo, &vkCommandBuffer_Array[i]);
        if(vkResult != VK_SUCCESS)
        {
            fprintf(gpFILE, "CreateCommandBuffers() : vkAllocateCommandBuffer() failed for iteration %d. (%d)\n", i, vkResult);
            return(vkResult);
        }
        else
        {
            fprintf(gpFILE, "CreateCommandBuffers() : vkAllocateCommandBuffer() succeeded for iteration %d\n",i);
        }
    }

     return (vkResult);
}

VkResult CreateRenderPass(void)
{
    VkResult vkResult = VK_SUCCESS;

    /*
    
     Step 1 : Declare and initialize VkAttachmentDescription Structure Array 
    
    */

    VkAttachmentDescription vkAttachmentDescription_Array[1];
    memset((void*)vkAttachmentDescription_Array, 0, sizeof(VkAttachmentDescription) * _ARRAYSIZE(vkAttachmentDescription_Array));
    vkAttachmentDescription_Array[0].flags          =   0;
    vkAttachmentDescription_Array[0].format         =   vkFormat_color;
    vkAttachmentDescription_Array[0].samples        =   VK_SAMPLE_COUNT_1_BIT;             // Image is not multisampling 1 bit is enough
    vkAttachmentDescription_Array[0].loadOp         =   VK_ATTACHMENT_LOAD_OP_CLEAR;       // Will clear this layer first and then load 
    vkAttachmentDescription_Array[0].storeOp        =   VK_ATTACHMENT_STORE_OP_STORE;
    vkAttachmentDescription_Array[0].stencilLoadOp  =   VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // Although its called stencil but its used for depth buffer also
    vkAttachmentDescription_Array[0].stencilStoreOp =   VK_ATTACHMENT_STORE_OP_DONT_CARE;
    vkAttachmentDescription_Array[0].initialLayout  =   VK_IMAGE_LAYOUT_UNDEFINED;         // how to treat data arrangement once it gets in. 
    vkAttachmentDescription_Array[0].finalLayout    =   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;   // while getting out as per image set image layout same as source image


    /*
    
    Step 2 : Declare and initialize VkAttachmentReference Structure which will have information about the attachment which we described above
    
    */
    VkAttachmentReference vkAttachmentReference ;
    memset((void *)&vkAttachmentReference, 0, sizeof(VkAttachmentReference));
    vkAttachmentReference.attachment = 0;                                       // index number of 0 from VkAttachmentDescription array;
    vkAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;    // how to keep layout (setting as color attachment)


    /*
    
    Step 3 : Declare and initialize VkSubpassDescription Structure and keep reference about VkAttachmentReference Structure
    
    */
    VkSubpassDescription vkSubpassDescription;
    memset((void *)&vkSubpassDescription, 0, sizeof(VkSubpassDescription));
    vkSubpassDescription.flags = 0;
    vkSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // tells that its part of graphics pipeline 
    vkSubpassDescription.inputAttachmentCount = 0;
    vkSubpassDescription.pInputAttachments = NULL;
    vkSubpassDescription.colorAttachmentCount = _ARRAYSIZE(vkAttachmentDescription_Array);
    vkSubpassDescription.pColorAttachments = &vkAttachmentReference;
    vkSubpassDescription.pResolveAttachments = NULL;
    vkSubpassDescription.pDepthStencilAttachment = NULL;
    vkSubpassDescription.preserveAttachmentCount = 0;
    vkSubpassDescription.pPreserveAttachments  = NULL;


    /*
    
    Step 4 : Declare and initialize VkRenderPassCreateInfo Structure and refer above VkAttachmentDescription and VkSubpassDestricption into it
    
    */
    VkRenderPassCreateInfo vkRenderPassCreateInfo;
    memset((void*)&vkRenderPassCreateInfo, 0, sizeof(VkRenderPassCreateInfo));
    vkRenderPassCreateInfo.sType  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    vkRenderPassCreateInfo.pNext = NULL;
    vkRenderPassCreateInfo.flags = 0;
    vkRenderPassCreateInfo.attachmentCount = _ARRAYSIZE(vkAttachmentDescription_Array);
    vkRenderPassCreateInfo.pAttachments = vkAttachmentDescription_Array;
    vkRenderPassCreateInfo.subpassCount = 1;
    vkRenderPassCreateInfo.pSubpasses = &vkSubpassDescription;
    vkRenderPassCreateInfo.dependencyCount = 0;
    vkRenderPassCreateInfo.pDependencies = NULL;


    /*
    
    Step 5 : Now vkCreateRenderPass() API to create the actual RenderPass.
    
    */
    vkResult = vkCreateRenderPass(vkDevice, &vkRenderPassCreateInfo, NULL, &vkRenderPass);
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "CreateRenderPass() : vkCreateRenderPass() failed %d.\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "CreateRenderPass() : vkCreateRenderPass() succeeded \n");
    }

    return (vkResult);
}

VkResult CreateFramebuffers(void)
{
    VkResult vkResult = VK_SUCCESS;

    /*
    
    Step 1 :  Declare an array of VkImageView equal to the number of Attachments means in our example array of 1(Array[1]).
    
    */
    VkImageView vkImageView_Attachments_Array[1];
    memset((void*)vkImageView_Attachments_Array, 0, sizeof(VkImageView) * _ARRAYSIZE(vkImageView_Attachments_Array));

    /*
    
    Step 2 : Declare and initialize VkFrameBufferCreateInfo Structure.
    
    */
    VkFramebufferCreateInfo vkFramebufferCreateInfo ;
    memset((void*)& vkFramebufferCreateInfo, 0, sizeof(VkFramebufferCreateInfo));
    vkFramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    vkFramebufferCreateInfo.pNext = NULL;
    vkFramebufferCreateInfo.flags = 0;
    vkFramebufferCreateInfo.renderPass = vkRenderPass;
    vkFramebufferCreateInfo.attachmentCount = _ARRAYSIZE(vkImageView_Attachments_Array);
    vkFramebufferCreateInfo.pAttachments = vkImageView_Attachments_Array;
    vkFramebufferCreateInfo.width = vkExtent2D_Swapchain.width;
    vkFramebufferCreateInfo.height = vkExtent2D_Swapchain.height;
    vkFramebufferCreateInfo.layers = 1;

    /*
    
    Step 3 : Allocate the frambuffer array by malloc equal to the size of swapchain image count.
    
    */
    vkFramebuffer_Array = (VkFramebuffer*)malloc(sizeof(VkFramebuffer) * swapchainImageCount);

    for(uint32_t i = 0; i < swapchainImageCount; i++)
    {
        vkImageView_Attachments_Array[0] = swapchainImageView_Array[i];

        vkResult = vkCreateFramebuffer(vkDevice, &vkFramebufferCreateInfo, NULL, &vkFramebuffer_Array[i]);
        if(vkResult != VK_SUCCESS)
        {
            fprintf(gpFILE, "CreateFramebuffers() : vkCreateFramebuffer() failed for iteration %d. (%d)\n", i, vkResult);
            return(vkResult);
        }
        else
        {
            fprintf(gpFILE, "CreateFramebuffers() : vkCreateFramebuffer() succeeded for iteration %d\n",i);
        }
    }

    

    return (vkResult);
}

VkResult CreateSemaphores(void)
{
    /*
    
        Step 2 : In CreateSemaphore used defined function declare , memset and initialize VkSemaphoreCreateInfo Structure.

    */
    VkResult vkResult = VK_SUCCESS;
    VkSemaphoreCreateInfo vkSemaphoreTypeCreateInfo;

    memset((void*)&vkSemaphoreTypeCreateInfo , 0, sizeof(VkSemaphoreTypeCreateInfo));

    vkSemaphoreTypeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkSemaphoreTypeCreateInfo.pNext = NULL;
    vkSemaphoreTypeCreateInfo.flags = 0;

    /*
    
        Step 3 : Now call vkCreateSemaphore 2 times to create our 2 semaphore objects.
            Remember both will use same VkSemaphoreCreateInfo Structure.


        if type of Semaphore is not specified then by default binary semaphore is created.
    */

    vkResult = vkCreateSemaphore(vkDevice, &vkSemaphoreTypeCreateInfo, NULL, &vkSemaphore_BackBuffer);
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "CreateSemaphores() : vkCreateSemaphore() vkSemaphore_BackBuffer failed %d.\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "CreateSemaphores() : vkCreateSemaphore() vkSemaphore_BackBuffer succeeded \n");
    }


    vkResult = vkCreateSemaphore(vkDevice, &vkSemaphoreTypeCreateInfo, NULL, &vkSemaphore_RenderComplete);
    if(vkResult != VK_SUCCESS)
    {
        fprintf(gpFILE, "CreateSemaphores() : vkCreateSemaphore() vkSemaphore_RenderComplete failed %d.\n", vkResult);
        return(vkResult);
    }
    else
    {
        fprintf(gpFILE, "CreateSemaphores() : vkCreateSemaphore() vkSemaphore_RenderComplete succeeded \n");
    }

    return (vkResult);
}


VkResult CreateFences(void)
{
    VkResult vkResult = VK_SUCCESS;

    /*
    
        Step 4 : In creteFences User Defined function declare , memset and initialize VkFenceCreateInfo Structure.

    */
    VkFenceCreateInfo vkFenceCreateInfo;
    memset((void*)&vkFenceCreateInfo, 0, sizeof(VkFenceCreateInfo));

    vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkFenceCreateInfo.pNext = NULL;
    vkFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT ;


    /*
    
        Step 5 : In This user defined function allocate our global fence array to the size of SwapchainImageCount using malloc.

    */
    vkFence_Array = (VkFence*) malloc(sizeof(VkFence) * swapchainImageCount);

    /*
    
        Step 6 : Now in a loop call vkCreateFence() to initialize our global fences array.
    
    */    
    for(uint32_t i = 0; i < swapchainImageCount; i++)
    {
        vkResult = vkCreateFence(vkDevice, &vkFenceCreateInfo, NULL, &vkFence_Array[i]);
        if(vkResult != VK_SUCCESS)
        {
            fprintf(gpFILE, "CreateFences() : vkCreateFence() failed for iteration %d. (%d)\n", i, vkResult);
            return(vkResult);
        }
        else
        {
            fprintf(gpFILE, "CreateFences() : vkCreateFence() succeeded for iteration %d\n", i);
        }

    }
    return(vkResult);
}


VkResult BuildCommandBuffers()
{
    VkResult vkResult = VK_SUCCESS;

    // Loop per swapchain Image
    for(uint32_t i = 0; i < swapchainImageCount; i++)
    {
        // Reset Command Buffers 
        vkResult = vkResetCommandBuffer(vkCommandBuffer_Array[i], 0);
        if(vkResult != VK_SUCCESS)
        {
            fprintf(gpFILE, "BuildCommandBuffers() : vkResetCommandBuffer() failed for iteration %d. (%d)\n", i, vkResult);
            return(vkResult);
        }
        else
        {
            fprintf(gpFILE, "BuildCommandBuffers() : vkResetCommandBuffer() succeeded for iteration %d\n", i);
        }

        VkCommandBufferBeginInfo vkCommandBufferBeginInfo ;

        memset((void*)&vkCommandBufferBeginInfo, 0, sizeof(VkCommandBufferBeginInfo));
        vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkCommandBufferBeginInfo.pNext = NULL;
        vkCommandBufferBeginInfo.flags = 0;

        vkResult = vkBeginCommandBuffer(vkCommandBuffer_Array[i], &vkCommandBufferBeginInfo);
        if(vkResult != VK_SUCCESS)
        {
            fprintf(gpFILE, "BuildCommandBuffers() : vkBeginCommandBuffer() failed for iteration %d. (%d)\n", i, vkResult);
            return(vkResult);
        }
        else
        {
            fprintf(gpFILE, "BuildCommandBuffers() : vkBeginCommandBuffer() succeeded for iteration %d\n", i);
        }

        // set clear values

        VkClearValue vkClearValue_Array[1];

        memset((void*)vkClearValue_Array, 0, sizeof(VkClearValue) * _ARRAYSIZE(vkClearValue_Array));

        vkClearValue_Array[0].color = vkClearColorValue;

        // RenderPass Begin Info
        VkRenderPassBeginInfo vkRenderPassBeginInfo;

        memset((void*)&vkRenderPassBeginInfo, 0 , sizeof(VkRenderPassBeginInfo));

        vkRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        vkRenderPassBeginInfo.pNext = NULL;
        vkRenderPassBeginInfo.renderPass = vkRenderPass;
        vkRenderPassBeginInfo.renderArea.offset.x = 0;
        vkRenderPassBeginInfo.renderArea.offset.y = 0;
        vkRenderPassBeginInfo.renderArea.extent.width = vkExtent2D_Swapchain.width;
        vkRenderPassBeginInfo.renderArea.extent.height = vkExtent2D_Swapchain.height;
        vkRenderPassBeginInfo.clearValueCount = _ARRAYSIZE(vkClearValue_Array);
        vkRenderPassBeginInfo.pClearValues = vkClearValue_Array;
        vkRenderPassBeginInfo.framebuffer = vkFramebuffer_Array[i];

        // Begin renderPass
        vkCmdBeginRenderPass(vkCommandBuffer_Array[i], &vkRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Here we should call vulkan drawing functions


        // End RenderPass
        vkCmdEndRenderPass(vkCommandBuffer_Array[i]);

        // End command Buffer Recording
        vkResult = vkEndCommandBuffer(vkCommandBuffer_Array[i]);
        if(vkResult != VK_SUCCESS)
        {
            fprintf(gpFILE, "BuildCommandBuffers() : vkEndCommandBuffer() failed for iteration %d. (%d)\n", i, vkResult);
            return(vkResult);
        }
        else
        {
            fprintf(gpFILE, "BuildCommandBuffers() : vkEndCommandBuffer() succeeded for iteration %d\n", i);
        }
    }
    return (vkResult);
}
