#include "Window.h"
//TODO: Implement Window class
Window::Window(HWND hwnd, const std::wstring &windowName, int clientwidth, int clientheight, bool vSync) :m_hWnd(hwnd), m_windowName(windowName), m_clientheight(clientheight), m_clientWidth(clientheight), m_vSync(vSync), m_fullscreen(false), m_frameCounter(0){

}
void Window::OnUpdate(double deltaTime, double totalTime){}

void Window::OnRender(double deltaTime, double totalTime){}

void Window::OnResize(int clientHeight, int clientWidth){}

Window::~Window(){}
