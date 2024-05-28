#include "Peer.h"
#include "DataSource.h"
#include <iostream>
#include <string>
#include <functional>
#include "ThreadPool.h"
#include "imgui.h"              // Include imgui library  lib\imgui\imgui.h
#include "imgui_impl_glfw.h"    // Include imgui_impl_glfw
#include "imgui_impl_opengl3.h" // Include imgui_impl_opengl3
#include "GLFW/glfw3.h"
#include "timer.h"

static int flag;
Timer timer;
static void
glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int Sync(Peer &peer)
{
    peer.syncDataSources();
    return 1;
}

/**
 * Function to display the menu and handle user interactions.
 *
 * @param peer Reference to the peer object.
 * @param log Reference to the log string.
 */
void displayMenu(Peer &peer, std::string &log)
{
    ImGui::Text("Distributed File Synchronization and Storage System");
    ImGui::Separator();

    static bool add_peer_popup = false;
    static bool add_data_source_popup = false;
    static bool set_sync_period_popup = false;
    static bool set_peer_path_popup = false;

    // Buttons to open popups
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
    if (ImGui::Button("Sync Data Sources"))
    {
        peer.syncDataSources();
        log += "Synced data sources\n";
    }
    if (ImGui::Button("Set Peer Path"))
    {
        set_peer_path_popup = true;
    }
    if (ImGui::Button("Exit"))
    {
        exit(0);
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
    if (set_peer_path_popup)
    {
        ImGui::OpenPopup("Set Peer Path");
        set_peer_path_popup = false;
    }

    // Add Peer popup
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
                if (flag) // todo：实现随时同步
                {
                    peer.syncDataSources();
                }
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

    // Add Data Source popup
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
                if (flag) // todo：实现随时同步
                {
                    peer.syncDataSources();
                }
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

    // Set Sync Period popup
    if (ImGui::BeginPopupModal("Set Sync Period", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static int period = 0;
        ImGui::RadioButton("Immediate", &period, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Minutely", &period, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Hourly", &period, 2);
        ImGui::NewLine();
        ImGui::RadioButton("Daily", &period, 3);
        ImGui::SameLine();
        ImGui::RadioButton("Weekly", &period, 4);
        ImGui::SameLine();
        ImGui::RadioButton("Monthly", &period, 5);
        if (ImGui::Button("Set"))
        {
            peer.setSyncPeriod(static_cast<SyncPeriod>(period));
            log += "Set sync period to " + std::to_string(period) + "\n";
            timer.stop();
            flag = 0;
            if (peer.Pgetter() == HOURLY)
            {
                timer.start(1000 * 60 * 60, std::bind(Sync, std::ref(peer)));
            }
            else if (peer.Pgetter() == MINUTELY)
            {
                timer.start(1000 * 60, std::bind(Sync, std::ref(peer)));
            }
            else if (peer.Pgetter() == QUATERLY)
            {
                timer.start(1000 * 60 * 15, std::bind(Sync, std::ref(peer)));
            }
            else if (peer.Pgetter() == DAILY)
            {
                timer.start(1000 * 60 * 60 * 24, std::bind(Sync, std::ref(peer)));
            }
            else if (peer.Pgetter() == WEEKLY)
            {
                timer.start(1000 * 60 * 60 * 24 * 7, std::bind(Sync, std::ref(peer)));
            }
            else if (peer.Pgetter() == MONTHLY)
            {
                timer.start(long(1000 * 60 * 60 * 24 * 10), std::bind(Sync, std::ref(peer)));
            }

            else
            {
                // timer.start(1000,std::bind(&Peer::syncDataSources, &peer));
                flag = 1;
            }
            if (flag) // todo：实现随时同步
            {
                peer.syncDataSources();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Set Peer Path popup
    if (ImGui::BeginPopupModal("Set Peer Path", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char path[128] = "";
        ImGui::InputText("Peer Path", path, IM_ARRAYSIZE(path));
        if (ImGui::Button("Set"))
        {
            int key = 0;
            key = peer.setPeerPath(path);
            path[0] = '\0';
            if (key == 1)
            {
                log += "Set peer path to " + std::string(path) + "\n";
                if (flag) // todo：实现随时同步
                {
                    peer.syncDataSources();
                }
                ImGui::CloseCurrentPopup();
            }
            else if (key == 0)
            {
                log += "Error: Path cannot be empty!\n";
            }
            else
            {
                log += "Error: No such dictionary!\n";
            }
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
    flag = 0;

    // if (peer.Pgetter() == HOURLY)
    // {
    //     timer.start(60000, std::bind(&Peer::syncDataSources, &peer));
    // }
    // else if (peer.Pgetter() == DAILY)
    // {
    //     timer.start(86400000, std::bind(&Peer::syncDataSources, &peer));
    // }
    // else
    // {
    //     // timer.start(1000, std::bind(&Peer::syncDataSources, &peer));
    //     flag = 1;
    // }

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
        static bool add_peer_popup = false;
        static bool add_data_source_popup = false;
        static bool set_sync_period_popup = false;
        if (ImGui::Button("Execute"))
        {
            // Execute command logic here
            log += "Executed command: " + std::string(input) + "\n";
            // 执行命令逻辑
            std::string command = std::string(input);
            if (command == "-a P")
            {
                add_peer_popup = true;
            }
            else if (command == "-a D")
            {
                add_data_source_popup = false;
            }

            else if (command == "-s Sync")
            {
                set_sync_period_popup = true;
            }
            else if (command == "-d kP")
            {
                std::string res = peer.displayNodeInfo();
                log += "Displayed peer info\n";
                log += res;
            }
            else if (command == "Exit")
            {
                exit(0);
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
                    timer.stop();
                    flag = 0;
                    if (peer.Pgetter() == HOURLY)
                    {
                        timer.start(60000, std::bind(&Peer::syncDataSources, &peer));
                    }
                    else if (peer.Pgetter() == DAILY)
                    {
                        timer.start(86400000, std::bind(&Peer::syncDataSources, &peer));
                    }
                    else
                    {
                        // timer.start(1000,std::bind(&Peer::syncDataSources, &peer));
                        flag = 1;
                    }

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
