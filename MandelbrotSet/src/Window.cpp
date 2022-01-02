#include "include/Window.h"
#include "include/Application.h"
#include <utility>
#include <assert.h>
#include <windowsx.h>

namespace Utilities {
	constexpr const char* g_WindowClassName = "Vulkan Mandelbrot Renderer";
	constexpr uint32_t EventHandled = 0;
	constexpr uint32_t MaxBufferLength = 128;
}

INTERNALSCOPE LRESULT CALLBACK Win32ProcedureEventFunctionCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INTERNALSCOPE LRESULT PASCAL Win32ProcedureErrorFunctionCallback(HWND hwnd, INT errorID);

Window::Window(const HINSTANCE hInstance, WindowProperties&& windowProperties) noexcept
	:
	m_Handle(NULL),
	m_HInstance(hInstance),
	m_WindowProperties(std::move(windowProperties)),
	m_KeyStates({ 0 })
{
	WNDCLASSEXA windowClass;
	windowClass.hInstance = m_HInstance;
	windowClass.lpszClassName = Utilities::g_WindowClassName;
	windowClass.lpszMenuName = NULL;
	windowClass.lpfnWndProc = Win32ProcedureEventFunctionCallback;
	windowClass.hIcon = LoadIconA(m_HInstance, reinterpret_cast<LPCSTR>(IDI_APPLICATION));
	windowClass.hIconSm = LoadIconA(m_HInstance, reinterpret_cast<LPCSTR>(IDI_APPLICATION));
	windowClass.hCursor = LoadCursorA(m_HInstance, reinterpret_cast<LPCSTR>(IDC_ARROW));
	windowClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	windowClass.style = NULL;
	windowClass.cbSize = sizeof(WNDCLASSEXA);
	windowClass.cbWndExtra = NULL;
	windowClass.cbClsExtra = NULL;

	if (!RegisterClassExA(&windowClass))
	{
		MessageBoxExA(NULL, "Failed to register window class", NULL, MB_ICONERROR, LANG_SYSTEM_DEFAULT);
		assert(false);
	}

	m_Handle = CreateWindowExA(
		WS_EX_LEFT,
		Utilities::g_WindowClassName,
		"Vulkan Mandelbrot Set Renderer",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		m_WindowProperties.Width,
		m_WindowProperties.Height,
		NULL,
		NULL,
		m_HInstance,
		this);

	if (!m_Handle)
	{
		MessageBoxExA(NULL, "Failed to create window", NULL, MB_ICONERROR, LANG_SYSTEM_DEFAULT);
		assert(false);
	}

	const uint32_t consoleFlags = m_WindowProperties.ShowCMD ? SW_SHOW : SW_HIDE;
	ShowWindow(GetConsoleWindow(), consoleFlags);

	constexpr uint32_t windowFlags = SW_SHOWMAXIMIZED;
	ShowWindow(m_Handle, windowFlags);
	UpdateWindow(m_Handle);
	SetFocus(m_Handle);
}

Window::~Window()
{
	if(m_Handle)
		DestroyWindow(m_Handle);
}

void Window::PollEvents()
{
	INTERNALSCOPE MSG message;
	while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
}

bool Window::KeyPressed(const KeyCode keyCode)
{
	return static_cast<bool>(m_KeyStates[static_cast<std::size_t>(keyCode)]);
}

const std::pair<uint32_t, uint32_t> Window::GetSize() const
{
	return { m_WindowProperties.Width, m_WindowProperties.Height };
}

const std::pair<HWND, HINSTANCE> Window::GetInternalState() const
{
	return { m_Handle, m_HInstance };
}

/* Callbacks */
LRESULT CALLBACK Win32ProcedureEventFunctionCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Window* window = reinterpret_cast<Window*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

	switch (message)
	{
		case WM_CREATE:
		{
			CREATESTRUCTA* pCreateStruct = reinterpret_cast<CREATESTRUCTA*>(lParam);
			Window* _UserData = reinterpret_cast<Window*>(pCreateStruct->lpCreateParams);
			SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(_UserData));
			window = reinterpret_cast<Window*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

			POINT point;
			GetCursorPos(reinterpret_cast<LPPOINT>(&point));
			return Utilities::EventHandled;
		}

		case WM_DESTROY:
		{
			PostQuitMessage(wParam);

			WindowCloseEvent event;
			window->m_WindowProperties.CallbackFunction(event);

			return Utilities::EventHandled;
		}

		case WM_CLOSE:
		{
			/* Fire close event */
			PostQuitMessage(wParam);
			WindowCloseEvent event;
			window->m_WindowProperties.CallbackFunction(event);

			return Utilities::EventHandled;
		}

		case WM_SIZE:
		{
			DWORD width = LOWORD(lParam);
			DWORD height = HIWORD(wParam);

			RECT rectangle;
			GetClientRect(hwnd, &rectangle);
			const uint32_t actualWidth = rectangle.right - rectangle.left;
			const uint32_t actualHeight = rectangle.bottom - rectangle.top;

			window->m_WindowProperties.Width = actualWidth;
			window->m_WindowProperties.Height = actualHeight;

			WindowResizeEvent event(actualWidth, actualHeight);
			window->m_WindowProperties.CallbackFunction(event);
			return Utilities::EventHandled;
		}

		case WM_ERASEBKGND:
		{
			/* Notify the OS that erasing will be handled by the application to prevent flicker. */
			/* return 1; */

			/* Works smoother */
			return DefWindowProcA(hwnd, message, wParam, lParam);
		}

		case WM_PAINT:
		{
			PAINTSTRUCT paintStruct;
			BeginPaint(hwnd, &paintStruct);
			HDC hdc = BeginPaint(hwnd, &paintStruct);
			FillRect(hdc, &paintStruct.rcPaint, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
			EndPaint(hwnd, &paintStruct);

			return Utilities::EventHandled;
		}

		default:
		{
			return DefWindowProcA(hwnd, message, wParam, lParam);
		}

		case WM_KEYDOWN:
		{
			window->m_KeyStates[static_cast<std::size_t>(wParam)] = static_cast<uint8_t>(true);
			return Utilities::EventHandled;
		}

		case WM_KEYUP:
		{
			window->m_KeyStates[static_cast<std::size_t>(wParam)] = static_cast<uint8_t>(false);
			return Utilities::EventHandled;
		}

		case WM_MBUTTONDOWN:
		{
			return Utilities::EventHandled;
		}

		case WM_MBUTTONUP:
		{
			return Utilities::EventHandled;
		}


		case WM_MOUSEMOVE:
		{
			const DWORD xPosition = GET_X_LPARAM(lParam);
			const DWORD yPosition = GET_Y_LPARAM(lParam);

			return Utilities::EventHandled;
		}
	}

	return DefWindowProcA(hwnd, message, wParam, lParam);
}

LRESULT PASCAL Win32ProcedureErrorFunctionCallback(HWND hwnd, INT errorID)
{
	INTERNALSCOPE TCHAR s_ErrorBuffer[Utilities::MaxBufferLength]{ 0 };

	if (!hwnd)
	{
		MessageBoxExA(
			NULL,
			"An Win32 API error occured, but the window is null!",
			NULL,
			MB_ICONERROR,
			LANG_SYSTEM_DEFAULT);

		return static_cast<LRESULT>(false);
	}

	if (errorID == 0)
		return static_cast<LRESULT>(true);

	//sprintf_s(s_ErrorBuffer, MaxBufferLength, "Error# %d", errorID);

	MessageBoxExA(
		hwnd,
		reinterpret_cast<LPCSTR>(s_ErrorBuffer),
		NULL,
		MB_ICONERROR,
		LANG_SYSTEM_DEFAULT);

	return static_cast<LRESULT>(true);
}