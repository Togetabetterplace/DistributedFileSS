#include "Peer.h"
#include "DataSource.h"
#include <iostream>
#include <string>
#include "ThreadPool.h"
#include <GLFW/glfw3.h> // Include GLFW library
#include "imgui/imgui.h" // Include imgui library
#include "imgui/backends/imgui_impl_glfw.h" // Include imgui_impl_glfw
#include "imgui/backends/imgui_impl_opengl3.h" // Include imgui_impl_opengl3

/g++ - Ilib / imgui - Ilib / imgui / backends - Ilib / imgui / examples / libs / glfw / include - Ilib / imgui / examples / libs / glad / include src / main.cpp src / Peer.cpp src / DataSource.cpp src / ThreadPool.cpp lib / imgui / imgui.cpp lib / imgui / imgui_demo.cpp lib / imgui / imgui_draw.cpp lib / imgui / imgui_tables.cpp lib / imgui / imgui_widgets.cpp lib / imgui / backends / imgui_impl_glfw.cpp lib / imgui / backends / imgui_impl_opengl3.cpp - Llib / imgui / examples / libs / glfw - lglfw - Llib / imgui / examples / libs / glad - lglad - lGL - ldl - lpthread - o DistributedProject

    static void
    glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void displayMenu(Peer &peer, std::string &log)
{
    ImGui::Text("Distributed File Synchronization and Storage System");
    ImGui::Separator();

    if (ImGui::Button("Add Peer"))
    {
        char peerAddress[128] = "";
        if (ImGui::InputText("Peer Address", peerAddress, IM_ARRAYSIZE(peerAddress), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            peer.addKnownPeer(peerAddress);
            log += "Added peer: " + std::string(peerAddress) + "\n";
        }
    }

    if (ImGui::Button("Add Data Source"))
    {
        char name[128] = "", path[128] = "", hash[128] = "";
        ImGui::InputText("Data Source Name", name, IM_ARRAYSIZE(name));
        ImGui::InputText("Data Source Path", path, IM_ARRAYSIZE(path));
        ImGui::InputText("Data Source Hash", hash, IM_ARRAYSIZE(hash));
        if (ImGui::Button("Confirm"))
        {
            peer.addDataSource(DataSource(name, path, hash));
            log += "Added data source: " + std::string(name) + "\n";
        }
    }

    if (ImGui::Button("Set Sync Period"))
    {
        static int period = 0;
        ImGui::RadioButton("Immediate", &period, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Hourly", &period, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Daily", &period, 2);
        if (ImGui::Button("Confirm Period"))
        {
            peer.setSyncPeriod(static_cast<SyncPeriod>(period));
            log += "Set sync period to " + std::to_string(period) + "\n";
        }
    }

    if (ImGui::Button("Display Peer Info"))
    {
        peer.displayNodeInfo();
        log += "Displayed peer info\n";
    }

    if (ImGui::Button("Exit"))
    {
        exit(0);
    }

    if (ImGui::Button("Sync Data Sources"))
    {
        peer.syncDataSources();
        log += "Synced data sources\n";
    }
}

int main(int, char **)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return -1;

    const char *glsl_version = "#version 130";
    GLFWwindow *window = glfwCreateWindow(1280, 720, "Distributed File Sync", NULL, NULL);
    if (window == NULL)
        return -1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    Peer peer(8080);
    std::string log;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Control Panel");
        displayMenu(peer, log);
        ImGui::End();

        ImGui::Begin("Log");
        ImGui::TextWrapped("%s", log.c_str());
        ImGui::End();

        ImGui::Begin("Command Input");
        static char input[128] = "";
        ImGui::InputText("Input", input, IM_ARRAYSIZE(input));
        if (ImGui::Button("Execute"))
        {
            // Execute command logic here
            log += "Executed command: " + std::string(input) + "\n";
            // Clear input
            input[0] = '\0';
        }
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
