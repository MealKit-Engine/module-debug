#include "mealkit/debug/ImGuiLayer.h"

#include "mealkit/core/Log.h"
#include "mealkit/platform/ImGuiPlatform.h"
#include "mealkit/platform/Paths.h"
#include "mealkit/platform/Window.h"
#include "mealkit/render/Renderer.h"
#include "mealkit/rhi/Rhi.h"

#include <cstring>
#include <imgui.h>
#include <imgui_internal.h> // RegisterUserTexture

namespace mealkit::debug {
namespace {
void applyStyle(float scale) {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    // Panels are separated by space and a darker gutter, not by lines
    s.WindowRounding = 6.0f;
    s.ChildRounding = 4.0f;
    s.PopupRounding = 6.0f;
    s.FrameRounding = 4.0f;
    s.GrabRounding = 3.0f;
    s.TabRounding = 4.0f;
    s.ScrollbarRounding = 6.0f;
    s.ScrollbarSize = 10.0f;
    s.WindowBorderSize = 1.0f;
    s.ChildBorderSize = 1.0f;
    s.PopupBorderSize = 1.0f;
    s.TabBarBorderSize = 0.0f;
    s.TabBarOverlineSize = 0.0f;
    s.DockingSeparatorSize = 4.0f;
    s.DockingNodeHasCloseButton = false; // each tab has its own close button
    s.TabCloseButtonMinWidthSelected = 0.0f; // tabs show it only while hovered
    s.TabCloseButtonMinWidthUnselected = 0.0f;
    s.WindowPadding = ImVec2(10.0f, 10.0f);
    s.FramePadding = ImVec2(7.0f, 4.0f);
    s.ItemSpacing = ImVec2(8.0f, 6.0f);
    s.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    s.CellPadding = ImVec2(6.0f, 3.0f);
    s.IndentSpacing = 16.0f;
    s.WindowMenuButtonPosition = ImGuiDir_None; // no triangle on every dock tab bar: tabs are all visible and can be dragged
    // Tokens of engine docs/TOOL_DESIGN.md 3.1: Radix slateDark (gray 1..12) and blueDark. Pick a step by its job, never a color by eye
    const auto rgb = [](u32 v, f32 a = 1.0f) {
        return ImVec4(((v >> 16) & 0xFF) / 255.0f, ((v >> 8) & 0xFF) / 255.0f, (v & 0xFF) / 255.0f, a);
    };
    const auto gray = [&](int step) {
        static constexpr u32 kSlate[13] = {0, 0x111113, 0x18191B, 0x212225, 0x272A2D, 0x2E3135, 0x363A3F,
                                           0x43484E, 0x5A6169, 0x696E77, 0x777B84, 0xB0B4BA, 0xEDEEF0};
        return rgb(kSlate[step]);
    };
    const ImVec4 clear(0.0f, 0.0f, 0.0f, 0.0f);
    const ImVec4 blue8 = rgb(0x2870BD); // focus, selection
    const ImVec4 blue9 = rgb(0x0090FF); // primary button, check marks
    ImVec4* c = s.Colors;
    c[ImGuiCol_Text] = gray(12);
    c[ImGuiCol_TextDisabled] = gray(11); // secondary text: paths, dates, hints
    c[ImGuiCol_TextSelectedBg] = rgb(0x2870BD, 0.45f);
    c[ImGuiCol_WindowBg] = gray(2);
    c[ImGuiCol_ChildBg] = clear;
    c[ImGuiCol_PopupBg] = gray(3);
    c[ImGuiCol_Border] = gray(6);
    c[ImGuiCol_BorderShadow] = clear;
    c[ImGuiCol_MenuBarBg] = gray(1);
    c[ImGuiCol_TitleBg] = gray(1);
    c[ImGuiCol_TitleBgActive] = gray(1);
    c[ImGuiCol_TitleBgCollapsed] = gray(1);
    // Hover is one neutral step up, pressed and selected one more; the accent never fills a hovered control
    c[ImGuiCol_FrameBg] = gray(3);
    c[ImGuiCol_FrameBgHovered] = gray(4);
    c[ImGuiCol_FrameBgActive] = gray(5);
    c[ImGuiCol_Header] = gray(5);
    c[ImGuiCol_HeaderHovered] = gray(4);
    c[ImGuiCol_HeaderActive] = gray(6);
    c[ImGuiCol_Button] = gray(3);
    c[ImGuiCol_ButtonHovered] = gray(4);
    c[ImGuiCol_ButtonActive] = gray(5);
    c[ImGuiCol_CheckMark] = blue9;
    c[ImGuiCol_SliderGrab] = gray(8);
    c[ImGuiCol_SliderGrabActive] = blue9;
    c[ImGuiCol_Separator] = gray(6);
    c[ImGuiCol_SeparatorHovered] = blue8;
    c[ImGuiCol_SeparatorActive] = blue9;
    c[ImGuiCol_ResizeGrip] = clear;
    c[ImGuiCol_ResizeGripHovered] = blue8;
    c[ImGuiCol_ResizeGripActive] = blue9;
    c[ImGuiCol_ScrollbarBg] = clear;
    c[ImGuiCol_ScrollbarGrab] = gray(6);
    c[ImGuiCol_ScrollbarGrabHovered] = gray(7);
    c[ImGuiCol_ScrollbarGrabActive] = gray(8);
    // The selected tab joins its panel; the others sit on the strip
    c[ImGuiCol_Tab] = clear;
    c[ImGuiCol_TabHovered] = gray(4);
    c[ImGuiCol_TabSelected] = gray(2);
    c[ImGuiCol_TabSelectedOverline] = clear;
    c[ImGuiCol_TabDimmed] = clear;
    c[ImGuiCol_TabDimmedSelected] = gray(2);
    c[ImGuiCol_TabDimmedSelectedOverline] = clear;
    c[ImGuiCol_TableHeaderBg] = gray(3);
    c[ImGuiCol_TableBorderStrong] = gray(6);
    c[ImGuiCol_TableBorderLight] = gray(5);
    c[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.015f);
    c[ImGuiCol_DockingPreview] = rgb(0x2870BD, 0.5f);
    c[ImGuiCol_DockingEmptyBg] = clear;
    c[ImGuiCol_NavCursor] = blue8;
    s.ScaleAllSizes(scale);
    s.FontScaleDpi = scale;
}
} // namespace

ImGuiLayer::~ImGuiLayer() { shutdown(); }

bool ImGuiLayer::init(Window& window, render::Renderer& renderer, const std::string& iniPath) {
    m_window = &window;
    m_renderer = &renderer;
    m_iniPath = iniPath;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    io.IniFilename = m_iniPath.empty() ? nullptr : m_iniPath.c_str();

    const float scale = imguiDpiScale(window);
    applyStyle(scale);

    // Merges Segoe UI for Latin and Malgun Gothic for Korean (Malgun Gothic alone shows backslash as ₩).
    // Since 1.92 glyphs are built on demand, so the full Hangul range isn't prebaked.
    bool haveFont = false;
    for (const char* fontPath : {"C:\\Windows\\Fonts\\segoeui.ttf", "C:\\Windows\\Fonts\\malgun.ttf"}) {
        if (!fileExists(fontPath)) continue;
        ImFontConfig config;
        config.MergeMode = haveFont;
        io.Fonts->AddFontFromFileTTF(fontPath, 16.0f, &config);
        haveFont = true;
    }
    if (!haveFont) io.Fonts->AddFontDefault();
    // Icon glyphs (icon::): Segoe Fluent Icons on Windows 11, Segoe MDL2 Assets on 10. Same code points
    for (const char* iconPath : {"C:\\Windows\\Fonts\\SegoeIcons.ttf", "C:\\Windows\\Fonts\\segmdl2.ttf"}) {
        if (!fileExists(iconPath)) continue;
        ImFontConfig merge;
        merge.MergeMode = true;
        merge.GlyphOffset = ImVec2(0.0f, 3.0f); // the icon font has a high baseline, so it's shifted down to the middle of Korean text lines
        io.Fonts->AddFontFromFileTTF(iconPath, 16.0f, &merge);
        break;
    }

    if (!imguiPlatformInit(window)) return false;
    if (!rhi::imguiInit(renderer.device())) return false;

    m_initialized = true;
    return true;
}

void ImGuiLayer::shutdown() {
    if (!m_initialized) return;
    rhi::imguiShutdown(m_renderer->device()); // also frees GPU resources of user textures
    if (m_window) imguiPlatformShutdown(*m_window);
    for (ImTextureData* tex : m_textures) {
        ImGui::UnregisterUserTexture(tex);
        IM_DELETE(tex);
    }
    m_textures.clear();
    ImGui::DestroyContext();
    m_initialized = false;
}

ImTextureData* ImGuiLayer::createTexture(const u8* rgba, u32 width, u32 height) {
    auto* tex = IM_NEW(ImTextureData)();
    tex->Create(ImTextureFormat_RGBA32, static_cast<int>(width), static_cast<int>(height));
    std::memcpy(tex->GetPixels(), rgba, static_cast<size_t>(width) * height * 4);
    ImGui::RegisterUserTexture(tex);
    m_textures.push_back(tex);
    return tex;
}

void ImGuiLayer::releaseTexture(ImTextureData* texture) {
    if (texture) texture->WantDestroyNextFrame = true; // the backend changes it to Destroyed after GPU use ends (deleted in newFrame)
}

bool ImGuiLayer::setIniPath(const std::string& iniPath) {
    const bool exists = fileExists(iniPath);
    if (!m_initialized || iniPath == m_iniPath) return exists;
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::GetCurrentContext()->SettingsLoaded) { // before the first frame, NewFrame reads the new file
        if (!m_iniPath.empty()) ImGui::SaveIniSettingsToDisk(m_iniPath.c_str());
        ImGui::ClearIniSettings();
        if (exists) ImGui::LoadIniSettingsFromDisk(iniPath.c_str());
    }
    m_iniPath = iniPath;
    io.IniFilename = m_iniPath.c_str();
    return exists;
}

void ImGuiLayer::newFrame(const std::function<void()>& inject) {
    for (size_t i = 0; i < m_textures.size();) {
        ImTextureData* tex = m_textures[i];
        if (tex->WantDestroyNextFrame && tex->Status == ImTextureStatus_Destroyed) {
            ImGui::UnregisterUserTexture(tex);
            IM_DELETE(tex);
            m_textures.erase(m_textures.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }
    rhi::imguiNewFrame();
    imguiPlatformNewFrame();
    if (inject) inject();
    ImGui::NewFrame();
}

void ImGuiLayer::render(render::Renderer& renderer, bool frameActive) {
    ImGui::Render();
    if (!frameActive) return;
    renderer.beginOverlay();
    rhi::imguiRender(renderer.device());
}

} // namespace mealkit::debug
