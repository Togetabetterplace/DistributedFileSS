#include "Peer.h"
#include "DataSource.h"
#include <iostream>
#include <string>
#include "ThreadPool.h"
#include "imgui.h"              // Include imgui library  lib\imgui\imgui.h
#include "imgui_impl_glfw.h"    // Include imgui_impl_glfw
#include "imgui_impl_opengl3.h" // Include imgui_impl_opengl3
#include "GLFW/glfw3.h"

/*

*/
static void
glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void displayMenu(Peer &peer, std::string &log)
{
    ImGui::Text("Distributed File Synchronization and Storage System");
    ImGui::Separator();

    static bool add_peer_popup = false;
    static bool add_data_source_popup = false;
    static bool set_sync_period_popup = false;

    if (ImGui::Button("Add Peer"))
    {
        add_peer_popup = true;
    }
    if (ImGui::Button("Add Data Source"))
    {
        add_data_source_popup = true;
    }
    if (ImGui::Button("Set Sync Period"))
    {
        set_sync_period_popup = true;
    }
    if (ImGui::Button("Display Peer Info"))
    {
        std::string res = peer.displayNodeInfo();
        log += "Displayed peer info\n";
        log += res;
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

    // Popups
    if (add_peer_popup)
    {
        ImGui::OpenPopup("Add Peer");
        add_peer_popup = false;
    }
    if (add_data_source_popup)
    {
        ImGui::OpenPopup("Add Data Source");
        add_data_source_popup = false;
    }
    if (set_sync_period_popup)
    {
        ImGui::OpenPopup("Set Sync Period");
        set_sync_period_popup = false;
    }

    if (ImGui::BeginPopupModal("Add Peer", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char peerAddress[128] = "";
        ImGui::InputText("Peer Address", peerAddress, IM_ARRAYSIZE(peerAddress));
        if (ImGui::Button("Add"))
        {
            if (strlen(peerAddress) > 0)
            {
                peer.addKnownPeer(peerAddress);
                log += "Added peer: " + std::string(peerAddress) + "\n";
                peerAddress[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            else
            {
                log += "Error: Peer Address cannot be empty\n";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Add Data Source", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char name[128] = "", path[128] = "", hash[128] = "";
        ImGui::InputText("Data Source Name", name, IM_ARRAYSIZE(name));
        ImGui::InputText("Data Source Path", path, IM_ARRAYSIZE(path));
        ImGui::InputText("Data Source Hash", hash, IM_ARRAYSIZE(hash));
        if (ImGui::Button("Add"))
        {
            if (strlen(name) > 0 && strlen(path) > 0 && strlen(hash) > 0)
            {
                peer.addDataSource(DataSource(name, path, hash));
                log += "Added data source: " + std::string(name) + "\n";
                name[0] = path[0] = hash[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            else
            {
                log += "Error: All fields are required\n";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Set Sync Period", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static int period = 0;
        ImGui::RadioButton("Immediate", &period, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Hourly", &period, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Daily", &period, 2);
        if (ImGui::Button("Set"))
        {
            peer.setSyncPeriod(static_cast<SyncPeriod>(period));
            log += "Set sync period to " + std::to_string(period) + "\n";
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
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

    // Adjust font size
    io.FontGlobalScale = 1.5f;

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

        // Adjust window layout
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(400, 720));
        ImGui::Begin("Control Panel", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        displayMenu(peer, log);
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(400, 0));
        ImGui::SetNextWindowSize(ImVec2(880, 360));
        ImGui::Begin("Log", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::TextWrapped("%s", log.c_str());
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(400, 360));
        ImGui::SetNextWindowSize(ImVec2(880, 360));
        ImGui::Begin("Command Input", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        static char input[128] = "";
        ImGui::InputText("Input", input, IM_ARRAYSIZE(input));
        if (ImGui::Button("Execute"))
        {
            // Execute command logic here
            log += "Executed command: " + std::string(input) + "\n";
            //todo:÷¥––√¸¡Ó
            command = std::string(input)£ª
             if (command == "-a")
            {

                if (type == "P")
                {
                    std::string peerAddress;
                    std::cout << "Enter peer address: ";
                    std::cin >> peerAddress;

                    /** Add a known peer to the peer's list of known peers */
                    peer.addKnownPeer(peerAddress);
                }
                else if (type == "D")
                {
                    std::string name, path, hash;
                    std::cout << "Enter data source name: ";
                    std::cin >> name;
                    std::cout << "Enter data source path: ";
                    std::cin >> path;
                    std::cout << "Enter data source hash: ";
                    std::cin >> hash;

                    /** Add a data source to the peer's list of data sources */
                    peer.addDataSource(DataSource(name, path, hash));
                }
            }
            else if (command == "-s")
            {
                std::string syncType;
                std::cin >> syncType;

                if (syncType == "Sync")
                {
                    int period;
                    std::cout << "Enter sync period (0: Immediate, 1: Hourly, 2: Daily): ";
                    std::cin >> period;

                    /** Set the synchronization period for the peer */
                    peer.setSyncPeriod(static_cast<SyncPeriod>(period));
                }
            }
            else if (command == "-d")
            {
                std::string type;
                std::cin >> type;

                if (type == "kP")
                {
                    /** Display information about the peer */
                    peer.displayNodeInfo();
                }
            }
            else if (command == "Exit")
            {
                break;
            }
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
