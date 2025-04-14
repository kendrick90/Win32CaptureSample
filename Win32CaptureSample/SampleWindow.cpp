#include "pch.h"
#include "App.h"
#include "SampleWindow.h"
#include "WindowList.h"
#include "MonitorList.h"
#include "ControlsHelper.h"
// SPOUT
#include "resource.h" // for version and icon

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Metadata;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::System;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
    using namespace Windows::UI::Composition::Desktop;
}

// SPOUT - change Class name from "Win32CaptureSample" to "SpoutWinCapture"
const std::wstring SampleWindow::ClassName = L"SpoutWinCapture";
namespace util
{
    using namespace robmikh::common::desktop::controls;
}

const std::wstring SampleWindow::ClassName = L"Win32CaptureSample.SampleWindow";
std::once_flag SampleWindowClassRegistration;

const std::wstring WindowsUniversalContract = L"Windows.Foundation.UniversalApiContract";

void SampleWindow::RegisterWindowClass()
{
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = instance;
    // SPOUT
    // wcex.hIcon = LoadIconW(instance, IDI_APPLICATION);
    wcex.hIcon = LoadIconW(instance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    // SPOUT
    // wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.hbrBackground = CreateSolidBrush(0x828484);
    wcex.lpszClassName = ClassName.c_str();
    // SPOUT
    // wcex.hIconSm = LoadIconW(wcex.hInstance, IDI_APPLICATION);
    wcex.hIconSm = LoadIconW(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));
    winrt::check_bool(RegisterClassExW(&wcex));
}

// SPOUT
// Allow for a command line, passed from main.cpp
// SampleWindow::SampleWindow(HINSTANCE instance, int cmdShow, std::shared_ptr<App> app)
SampleWindow::SampleWindow(HINSTANCE instance, LPSTR lpCmdLine, int cmdShow, std::shared_ptr<App> app)
{
    WINRT_ASSERT(!m_window);
    // SPOUT - change title from "Win32CaptureSample" to "SpoutWinCapture"
    WINRT_VERIFY(CreateWindowW(ClassName.c_str(), L"SpoutWinCapture",
                               WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 480,
                               nullptr, nullptr, instance, this));
    WINRT_ASSERT(m_window);

    //
    // SPOUT - centre the window and look for command line capture
    //
    // ===========================================================================
    //
    // Centre the window on the desktop work area
    //
    RECT rc = {0, 0, 800, 480}; // Client size
    GetWindowRect(m_window, &rc);
    RECT WorkArea;
    int WindowPosLeft = 0;
    int WindowPosTop = 0;
    SystemParametersInfo(SPI_GETWORKAREA, 0, (LPVOID)&WorkArea, 0);
    WindowPosLeft += ((WorkArea.right - WorkArea.left) - (rc.right - rc.left)) / 2;
    WindowPosTop += ((WorkArea.bottom - WorkArea.top) - (rc.bottom - rc.top)) / 2;
    MoveWindow(m_window, WindowPosLeft, WindowPosTop, (rc.right - rc.left), (rc.bottom - rc.top), false);
    //
    // Command line capture
    //
    // Find the window or display now so the main window
    // can be minimized after the initializations are done
    HWND hwndcapture = nullptr;
    int monitorindex = -1; // Monitor index
    if (lpCmdLine && *lpCmdLine)
    {
        // Remove double quotes
        std::string str = lpCmdLine;
        str.erase(std::remove(str.begin(), str.end(), '"'), str.end());
        if (str.length() == 1)
        {
            // Single character is a display - 0, 1, 2 etc
            monitorindex = atoi(str.c_str());
        }
        else
        {
            // Multiple characters - Window caption
            // Find the window handle from it's caption
            hwndcapture = FindWindowA(NULL, str.c_str());
        }
    }
    // ===========================================================================

    auto isAllDisplaysPresent = winrt::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 9);
    // SampleWindow::SampleWindow(int width, int height, std::shared_ptr<App> app)
    // {
    //     auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));

    //     std::call_once(SampleWindowClassRegistration, []() { RegisterWindowClass(); });

    //     auto exStyle = 0;
    //     auto style = WS_OVERLAPPEDWINDOW;

    //     RECT rect = { 0, 0, width, height };
    //     winrt::check_bool(AdjustWindowRectEx(&rect, style, false, exStyle));
    //     auto adjustedWidth = rect.right - rect.left;
    //     auto adjustedHeight = rect.bottom - rect.top;

    //     winrt::check_bool(CreateWindowExW(exStyle, ClassName.c_str(), L"Win32CaptureSample", style,
    //         CW_USEDEFAULT, CW_USEDEFAULT, adjustedWidth, adjustedHeight, nullptr, nullptr, instance, this));
    //     WINRT_ASSERT(m_window);

    //     auto isAllDisplaysPresent = winrt::ApiInformation::IsApiContractPresent(WindowsUniversalContract, 9);

    m_app = app;
    m_app->InitializeWithWindow(m_window);
    m_windows = std::make_unique<WindowList>();
    m_monitors = std::make_unique<MonitorList>(isAllDisplaysPresent);
    // SPOUT - pixel format selection not used
    // m_pixelFormats =
    // {
    // { L"B8G8R8A8UIntNormalized", winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized },
    // { L"R16G16B16A16Float", winrt::DirectXPixelFormat::R16G16B16A16Float }
    // };

    CreateControls(instance);

    // SPOUT - command line processing
    // ============================================
    // Disable the monitor index if greater than the monitor list size
    // "All displays" is the last index which also shows this window
    // on the main display, even if minimized
    if (monitorindex > (int)m_monitors->GetCurrentMonitors().size())
    {
        monitorindex = -1;
    }

    // Minimize the main window if a command line was found
    if (hwndcapture > 0 || monitorindex >= 0)
        ShowWindow(m_window, SW_MINIMIZE);
    else
        ShowWindow(m_window, cmdShow);
    UpdateWindow(m_window);

    // Command line capture
    if (hwndcapture > 0)
    {
        //
        // The windows combobox control has been created and populated
        // Set the current selection to the capture window
        //
        auto window = WindowInfo(hwndcapture);
        if (m_windows->GetCurrentWindows().size() > 0)
        {
            for (size_t i = 0; i < m_windows->GetCurrentWindows().size(); i++)
            {
                if (window == m_windows->GetCurrentWindows()[i])
                {
                    SendMessageW(m_windowComboBox, CB_SETCURSEL, i, 0);
                }
            }
        }
        //
        // Restore if minimized
        //
        if (IsIconic(hwndcapture))
            ShowWindow(hwndcapture, SW_RESTORE);
        //
        // TryStartCapture for the window
        //
        auto item = m_app->TryStartCaptureFromWindowHandle(hwndcapture);
        if (item != nullptr)
        {
            // Capture from this window straight away
            OnCaptureStarted(item, CaptureType::ProgrammaticWindow);
        }
    }
    else if (monitorindex >= 0)
    {
        //
        // Display capture
        //
        // Get the monitor details for the index
        auto monitor = m_monitors->GetCurrentMonitors()[monitorindex];
        // TryStartCapture for the monitor
        auto item = m_app->TryStartCaptureFromMonitorHandle(monitor.MonitorHandle);
        if (item != nullptr)
        {
            // Capture from this monitor straight away
            OnCaptureStarted(item, CaptureType::ProgrammaticMonitor);
        }
    }
    // ============================================

    m_pixelFormats =
        {
            {L"B8G8R8A8UIntNormalized", winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized},
            {L"R16G16B16A16Float", winrt::DirectXPixelFormat::R16G16B16A16Float}};
    m_dirtyRegionModes =
        {
            {L"Report Only", winrt::GraphicsCaptureDirtyRegionMode::ReportOnly},
            {L"Report and Render", winrt::GraphicsCaptureDirtyRegionMode::ReportAndRender}};
    m_updateIntervals =
        {
            {L"None", winrt::TimeSpan{0}},
            {L"1s", std::chrono::seconds(1)},
            {L"5s", std::chrono::seconds(5)},
        };

    CreateControls(instance);

    ShowWindow(m_window, SW_SHOW);
    UpdateWindow(m_window);
}

SampleWindow::~SampleWindow()
{
    m_windows.reset();
}

LRESULT SampleWindow::MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        auto command = HIWORD(wparam);
        auto hwnd = (HWND)lparam;

        switch (command)
        {

        case CBN_SELCHANGE:
        {
            auto index = SendMessageW(hwnd, CB_GETCURSEL, 0, 0);
            if (hwnd == m_windowComboBox)
            {
                auto window = m_windows->GetCurrentWindows()[index];
                auto item = m_app->TryStartCaptureFromWindowHandle(window.WindowHandle);
                if (item != nullptr)
                {
                    OnCaptureStarted(item, CaptureType::ProgrammaticWindow);
                }
                else
                {
                    StopCapture();
                }
            }
            else if (hwnd == m_monitorComboBox)
            {
                auto monitor = m_monitors->GetCurrentMonitors()[index];
                auto item = m_app->TryStartCaptureFromMonitorHandle(monitor.MonitorHandle);
                if (item != nullptr)
                {
                    OnCaptureStarted(item, CaptureType::ProgrammaticMonitor);
                }
                else
                {
                    StopCapture();
                }
            }
            else if (hwnd == m_pixelFormatComboBox)
            {
                auto pixelFormatData = m_pixelFormats[index];
                m_app->PixelFormat(pixelFormatData.PixelFormat);
            }
            else if (hwnd == m_dirtyRegionModeComboBox)
            {
                auto mode = m_dirtyRegionModes[index];
                m_app->DirtyRegionMode(mode.Mode);
            }
            else if (hwnd == m_minUpdateIntervalComboBox)
            {
                auto interval = m_updateIntervals[index];
                m_app->MinUpdateInterval(interval.Interval);
            }
        }
        break;

        case BN_CLICKED:
        {
            if (hwnd == m_pickerButton)
            {
                OnPickerButtonClicked();
            }
            else if (hwnd == m_stopButton)
            {
                StopCapture();
            }
            else if (hwnd == m_snapshotButton)
            {
                OnSnapshotButtonClicked();
            }
            else if (hwnd == m_cursorCheckBox)
            {
                auto value = SendMessageW(m_cursorCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                m_app->IsCursorEnabled(value);
            }
            // SPOUT client area capture
            else if (hwnd == m_clientCheckBox)
            {
                auto value = SendMessageW(m_clientCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                m_app->IsClientEnabled(value);
            }
            else if (hwnd == m_captureExcludeCheckBox)
            {
                auto value = SendMessageW(m_captureExcludeCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                winrt::check_bool(SetWindowDisplayAffinity(m_window, value ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE));
            }
            else if (hwnd == m_aboutButton)
            {
                // SPOUT
                std::string str = "\n                  SpoutWinCapture\n\n";
                str += "    Windows monitor and window capture\n";
                str += "   with Spout output, using <a href=\"https://learn.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/\">WinRT</a> methods\n";
                str += "      Original code by <a href=\"https://github.com/robmikh/Win32CaptureSample\">Robert Mikhayelyan</a>\n\n";
                str += "                       Version 2.0.0.5\n";
                str += "                  <a href=\"https://spout.zeal.co\">https://spout.zeal.co</a>\n";
                str += " \n";
                // Icon from resources
                SpoutMessageBoxIcon(LoadIconA(GetModuleHandle(NULL), MAKEINTRESOURCEA(IDI_ICON1)));
                // Centre on the application window
                SpoutMessageBox(m_window, str.c_str(), "About", MB_USERICON | MB_OK);
                else if (hwnd == m_borderRequiredCheckBox)
                {
                    auto value = SendMessageW(m_borderRequiredCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                    m_app->IsBorderRequired(value);
                }
                else if (hwnd == m_secondaryWindowsCheckBox)
                {
                    auto value = SendMessageW(m_secondaryWindowsCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                    m_app->IncludeSecondaryWindows(value);
                }
                else if (hwnd == m_visualizeDirtyRegionCheckBox)
                {
                    auto value = SendMessageW(m_visualizeDirtyRegionCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                    m_app->VisualizeDirtyRegions(value);
                }
            }
        }
        break;
        }
        break;
    case WM_DISPLAYCHANGE:
        m_monitors->Update();
        break;

    case WM_CTLCOLORSTATIC:
        return util::StaticControlColorMessageHandler(wparam, lparam);
    default:
        return base_type::MessageHandler(message, wparam, lparam);
    }

        return 0;
    }

    void SampleWindow::OnCaptureStarted(winrt::GraphicsCaptureItem const &item, CaptureType captureType)
    {
        m_itemClosedRevoker.revoke();
        m_itemClosedRevoker = item.Closed(winrt::auto_revoke, {this, &SampleWindow::OnCaptureItemClosed});
        SetSubTitle(std::wstring(item.DisplayName()));

        switch (captureType)
        {
        case CaptureType::ProgrammaticWindow:
            SendMessageW(m_monitorComboBox, CB_SETCURSEL, -1, 0);
            if (m_isSecondaryWindowsFeaturePresent)
            {
                EnableWindow(m_secondaryWindowsCheckBox, true);
            }
            break;
        case CaptureType::ProgrammaticMonitor:
            SendMessageW(m_windowComboBox, CB_SETCURSEL, -1, 0);
            if (m_isSecondaryWindowsFeaturePresent)
            {
                EnableWindow(m_secondaryWindowsCheckBox, false);
            }
            break;
        case CaptureType::Picker:
            SendMessageW(m_windowComboBox, CB_SETCURSEL, -1, 0);
            SendMessageW(m_monitorComboBox, CB_SETCURSEL, -1, 0);
            if (m_isSecondaryWindowsFeaturePresent)
            {
                // Check to see if the item is a window
                if (auto windowItem = item.try_as<IWindowGraphicsCaptureItemInterop>())
                {
                    EnableWindow(m_secondaryWindowsCheckBox, true);
                }
                else
                {
                    EnableWindow(m_secondaryWindowsCheckBox, false);
                }
            }
            break;
        }

        // SPOUT
        // SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_UNCHECKED, 0); // SPOUT - default no cursor
        m_app->IsCursorEnabled(false);
        SendMessageW(m_clientCheckBox, BM_SETCHECK, BST_CHECKED, 0); // SPOUT - default send client area

        SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(m_borderRequiredCheckBox, BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(m_visualizeDirtyRegionCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(m_dirtyRegionModeComboBox, CB_SETCURSEL, 0, 0);
        SendMessageW(m_minUpdateIntervalComboBox, CB_SETCURSEL, 0, 0);
        EnableWindow(m_stopButton, true);

        EnableWindow(m_snapshotButton, true);
    }

    winrt::fire_and_forget SampleWindow::OnPickerButtonClicked()
    {
        auto selectedItem = co_await m_app->StartCaptureWithPickerAsync();
        if (selectedItem)
        {
            OnCaptureStarted(selectedItem, CaptureType::Picker);
        }
    }

    winrt::fire_and_forget SampleWindow::OnSnapshotButtonClicked()
    {
        auto file = co_await m_app->TakeSnapshotAsync();

        // SPOUT
        // Replace image display with messagebox
        std::string str = winrt::to_string(file.Path());
        SpoutMessageBoxWindow(m_window); // Centre on the application window
        SpoutMessageBox(NULL, str.c_str(), "Saved image file", MB_ICONINFORMATION | MB_OK, 4000);

        // if (file != nullptr)
        // {
        // co_await winrt::Launcher::LaunchFileAsync(file);
        // }
    }

    // Not DPI aware but could be by multiplying the constants based on the monitor scale factor
    void SampleWindow::CreateControls(HINSTANCE instance)
    {
        // Programmatic capture
        auto isWin32ProgrammaticPresent = winrt::ApiInformation::IsApiContractPresent(WindowsUniversalContract, 8);
        auto win32ProgrammaticStyle = isWin32ProgrammaticPresent ? 0 : WS_DISABLED;

        // Cursor capture
        auto isCursorEnablePresent = winrt::ApiInformation::IsApiContractPresent(WindowsUniversalContract, 9);
        auto cursorEnableStyle = isCursorEnablePresent ? 0 : WS_DISABLED;

        // Window exclusion
        auto isWin32CaptureExcludePresent = winrt::ApiInformation::IsApiContractPresent(WindowsUniversalContract, 9);

        // Border configuration
        auto isBorderRequiredPresent = winrt::ApiInformation::IsPropertyPresent(winrt::name_of<winrt::GraphicsCaptureSession>(), L"IsBorderRequired");
        auto borderEnableSytle = isBorderRequiredPresent ? 0 : WS_DISABLED;

        // Dirty region mode
        auto isDirtyRegionModePresent = winrt::ApiInformation::IsPropertyPresent(winrt::name_of<winrt::GraphicsCaptureSession>(), L"DirtyRegionMode");
        auto dirtyRegionStyle = isDirtyRegionModePresent ? 0 : WS_DISABLED;

        // Min update interval
        auto isMinUpdateIntervalPresent = winrt::ApiInformation::IsPropertyPresent(winrt::name_of<winrt::GraphicsCaptureSession>(), L"MinUpdateInterval");
        auto minUpdateIntervalStyle = isMinUpdateIntervalPresent ? 0 : WS_DISABLED;

        // Secondary windows configuration
        m_isSecondaryWindowsFeaturePresent = winrt::ApiInformation::IsPropertyPresent(winrt::name_of<winrt::GraphicsCaptureSession>(), L"IncludeSecondaryWindows");

        auto controls = util::StackPanel(m_window, instance, 10, 10, 40, 200, 30);

        auto windowLabel = controls.CreateControl(util::ControlType::Label, L"Windows:");

        // Create window combo box
        m_windowComboBox = controls.CreateControl(util::ControlType::ComboBox, L"", win32ProgrammaticStyle);

        // Populate window combo box and register for updates
        m_windows->RegisterComboBoxForUpdates(m_windowComboBox);

        auto monitorLabel = controls.CreateControl(util::ControlType::Label, L"Displays:");

        // Create monitor combo box
        m_monitorComboBox = controls.CreateControl(util::ControlType::ComboBox, L"", win32ProgrammaticStyle);

        // Populate monitor combo box
        m_monitors->RegisterComboBoxForUpdates(m_monitorComboBox);

        // Create picker button
        m_pickerButton = controls.CreateControl(util::ControlType::Button, L"Open Picker");

        // Create stop capture button
        m_stopButton = controls.CreateControl(util::ControlType::Button, L"Stop Capture", WS_DISABLED);

        // Create independent snapshot button
        m_snapshotButton = controls.CreateControl(util::ControlType::Button, L"Take Snapshot", WS_DISABLED);

        //  // Create cursor checkbox
        // auto cursorCheckBox = controls.CreateControl(ControlType::CheckBox, L"Enable Cursor", cursorEnableStyle);

        // // The default state is true for cursor rendering
        // // SendMessageW(cursorCheckBox, BM_SETCHECK, BST_CHECKED, 0);
        // // SPOUT - default no cursor
        // SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);

        // SPOUT
        // Create send client area checkbox
        auto clientCheckBox = controls.CreateControl(ControlType::CheckBox, L"Send client area", cursorEnableStyle);
        // The default state is true
        SendMessageW(clientCheckBox, BM_SETCHECK, BST_CHECKED, 0);
        auto pixelFormatLabel = controls.CreateControl(util::ControlType::Label, L"Pixel Format:");

        // Create pixel format combo box
        m_pixelFormatComboBox = controls.CreateControl(util::ControlType::ComboBox, L"");

        // Populate pixel format combo box
        for (auto &pixelFormat : m_pixelFormats)
        {
            SendMessageW(m_pixelFormatComboBox, CB_ADDSTRING, 0, (LPARAM)pixelFormat.Name.c_str());
        }

        // The default pixel format is BGRA8
        SendMessageW(m_pixelFormatComboBox, CB_SETCURSEL, 0, 0);

        // Create cursor checkbox
        m_cursorCheckBox = controls.CreateControl(util::ControlType::CheckBox, L"Enable Cursor", cursorEnableStyle);

        // The default state is true for cursor rendering
        SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_CHECKED, 0);

        // Create capture exclude checkbox
        // NOTE: We don't version check this feature because setting WDA_EXCLUDEFROMCAPTURE is the same as
        //       setting WDA_MONITOR on older builds of Windows. We're changing the label here to try and
        //       limit any user confusion.
        std::wstring excludeCheckBoxLabel = isWin32CaptureExcludePresent ? L"Exclude this window" : L"Block this window";
        m_captureExcludeCheckBox = controls.CreateControl(util::ControlType::CheckBox, excludeCheckBoxLabel.c_str());

        // The default state is false for capture exclusion
        SendMessageW(m_captureExcludeCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);

        //
        // SPOUT
        // About button
        auto aboutButton = controls.CreateControl(ControlType::Button, L"About", WS_VISIBLE);

        m_windowComboBox = windowComboBox;
        m_monitorComboBox = monitorComboBox;
        m_pickerButton = pickerButton;
        m_stopButton = stopButton;
        m_snapshotButton = snapshotButton;
        m_cursorCheckBox = cursorCheckBox;
        m_captureExcludeCheckBox = captureExcludeCheckBox;
        m_clientCheckBox = clientCheckBox; // SPOUT Add a client/window capture checkbox
        // SPOUT
        m_aboutButton = aboutButton;

        // Border required checkbox
        m_borderRequiredCheckBox = controls.CreateControl(util::ControlType::CheckBox, L"Border required", borderEnableSytle);

        // The default state is true for border required checkbox
        SendMessageW(m_borderRequiredCheckBox, BM_SETCHECK, BST_CHECKED, 0);

        // Include secondary windows checkbox
        // NOTE: We always start disabled until a window capture is started
        m_secondaryWindowsCheckBox = controls.CreateControl(util::ControlType::CheckBox, L"Include secondary windows", WS_DISABLED);

        // The default state is false for the secondary windows checkbox
        SendMessageW(m_secondaryWindowsCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);

        // Border required checkbox
        m_visualizeDirtyRegionCheckBox = controls.CreateControl(util::ControlType::CheckBox, L"Visualize dirty region", dirtyRegionStyle);

        // The default state is false for dirty region checkbox
        SendMessageW(m_visualizeDirtyRegionCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);

        auto dirtyRegionModeLabel = controls.CreateControl(util::ControlType::Label, L"Dirty region mode:");

        // Create the dirty region mode combo box
        m_dirtyRegionModeComboBox = controls.CreateControl(util::ControlType::ComboBox, L"", dirtyRegionStyle);

        // Populate the dirty region mode combo box
        for (auto &mode : m_dirtyRegionModes)
        {
            SendMessageW(m_dirtyRegionModeComboBox, CB_ADDSTRING, 0, (LPARAM)mode.Name.c_str());
        }

        // The default dirty region mode is ReportOnly (index 0)
        SendMessageW(m_dirtyRegionModeComboBox, CB_SETCURSEL, 0, 0);

        auto minUpdateIntervalLabel = controls.CreateControl(util::ControlType::Label, L"Min update interval:");

        // Create the min update interval combo box
        m_minUpdateIntervalComboBox = controls.CreateControl(util::ControlType::ComboBox, L"", minUpdateIntervalStyle);

        // Populate the min update interval mode combo box
        for (auto &data : m_updateIntervals)
        {
            SendMessageW(m_minUpdateIntervalComboBox, CB_ADDSTRING, 0, (LPARAM)data.Name.c_str());
        }

        // The default min update interval is None (index 0)
        SendMessageW(m_minUpdateIntervalComboBox, CB_SETCURSEL, 0, 0);
    }

    void SampleWindow::SetSubTitle(std::wstring const &text)
    {
        // SPOUT
        // Change name from "Win32CaptureSample" to "SpoutWinCapture"
        std::wstring titleText(L"SpoutWinCapture");
        if (!text.empty())
        {
            titleText += (L" - " + text);
        }
        SetWindowTextW(m_window, titleText.c_str());
    }

    void SampleWindow::StopCapture()
    {
        m_app->StopCapture();
        SetSubTitle(L"");
        SendMessageW(m_windowComboBox, CB_SETCURSEL, -1, 0);
        SendMessageW(m_monitorComboBox, CB_SETCURSEL, -1, 0);
        // SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_UNCHECKED, 0); // SPOUT - default no cursor
        SendMessageW(m_clientCheckBox, BM_SETCHECK, BST_CHECKED, 0);   // SPOUT - default send client area

        // SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(m_borderRequiredCheckBox, BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(m_secondaryWindowsCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(m_visualizeDirtyRegionCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(m_dirtyRegionModeComboBox, CB_SETCURSEL, 0, 0);
        SendMessageW(m_minUpdateIntervalComboBox, CB_SETCURSEL, 0, 0);
        EnableWindow(m_stopButton, false);
        EnableWindow(m_snapshotButton, false);
    }

    void SampleWindow::OnCaptureItemClosed(winrt::GraphicsCaptureItem const &, winrt::IInspectable const &)
    {
        StopCapture();
    }