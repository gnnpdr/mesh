#include "viewer/mesh_viewer.hpp"
#include <chrono>
#include <thread>

void MeshViewer::MeshViewer::start_viewer() 
{
    polyscope::init();

    if (!orig_mesh_.is_empty())
    {
        auto original_obj = std::make_unique<SceneObject>();
        original_obj->name = "original";
        original_obj->mesh = orig_mesh_;
        original_obj->orig_mesh = orig_mesh_;
        original_obj->is_original = true;
        original_obj->color = Vec3f(0.8f, 0.8f, 0.8f);

        register_object(original_obj.get());
        selected_object_ = original_obj.get();
        objects_.push_back(std::move(original_obj));
    }
    else
    {
        std::cout << "No initial mesh loaded. Use 'Load OBJ File' to add models." << std::endl;
    }

    polyscope::state::userCallback = [this]() 
    {
        this->draw_ui();
    };

    polyscope::show();
}

//----UI-----------------------------------------------------------------------------------

void MeshViewer::MeshViewer::update_detail_range() 
{
    if (orig_mesh_.is_empty()) 
    {
        min_detail_ = 0.0f;
        max_detail_ = 1.0f;
        return;
    }
    
    int orig_faces = orig_mesh_.get_triang_amt();
    if (orig_faces == 0) 
    {
        min_detail_ = 0.0f;
        max_detail_ = 1.0f;
        return;
    }
    
    min_detail_ = 0.0f;
    int target_faces = 3;
    max_detail_ = 1.0f - (float)target_faces / orig_faces - 0.5f;
    
    if (max_detail_ < 0.01f) max_detail_ = 0.01f;
    if (max_detail_ > 1.0f) max_detail_ = 1.0f;

    if (detail_level_ > max_detail_)
        detail_level_ = max_detail_;
    if (detail_level_ < min_detail_)
        detail_level_ = min_detail_;
}

//---Tracing------------------------------------------------------------------------------------

void MeshViewer::MeshViewer::render_raytraced(const std::string& photo_name) 
{
    if (objects_.empty()) 
    {
        std::cerr << "No objects to render" << std::endl;
        return;
    }
    
    glm::vec3 cam_global_pos = polyscope::view::getCameraWorldPosition();
    glm::mat4 view_matrix = polyscope::view::getCameraViewMatrix();
    glm::mat4 inverted_view_matrix = glm::inverse(view_matrix);
        
    glm::vec3 forward = glm::normalize(glm::vec3(inverted_view_matrix[2]));
    glm::vec3 up = glm::normalize(glm::vec3(inverted_view_matrix[1]));
    glm::vec3 right = glm::normalize(glm::cross(forward, up));

    auto group_bounding_box = Mesh::compute_bounding_box_of_group(objects_, 
        [](const auto& ptr) -> const Mesh::Mesh& { return ptr->mesh; });
    
    Vec3f model_center((group_bounding_box[0].x() + group_bounding_box[1].x()) / 2, 
                       (group_bounding_box[0].y() + group_bounding_box[1].y()) / 2,
                       (group_bounding_box[0].z() + group_bounding_box[1].z()) / 2);

    RayTracer::Settings settings;
    settings.camera_position = Vec3f(cam_global_pos.x, cam_global_pos.y, cam_global_pos.z);
    settings.camera_target = Vec3f(model_center.x(), model_center.y(), model_center.z());
    settings.camera_up = Vec3f(up.x, up.y, up.z);
    settings.vertical_fov_deg = 45.0f;
    settings.width = 800;
    settings.height = 600;
    settings.light_pos = light_pos_;
    
    // Создаём ray tracer
    auto rt = std::make_shared<RayTracer::RayTracer>(settings);
    
    // Добавляем объекты с учётом их позиции
    for (const auto& obj : objects_) 
    {
        if (!obj->visible) continue;
        
        std::vector<Vec3f> transformed_vertices;
        auto vertices = obj->mesh.get_vertices();
        transformed_vertices.reserve(vertices.size());
        
        for (const auto& v : vertices) 
        {
            transformed_vertices.push_back(Vec3f(
                v.x() + obj->position.x(),
                v.y() + obj->position.y(),
                v.z() + obj->position.z()
            ));
        }
        
        Mesh::Mesh transformed_mesh = obj->mesh;
        transformed_mesh.set_vertices(transformed_vertices);
        rt->add_object(transformed_mesh);
    }

    // Запускаем рендер в отдельном потоке
    std::thread render_thread([rt, photo_name]() 
    {
        auto start = std::chrono::high_resolution_clock::now();
        rt->render(photo_name);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    });
    render_thread.detach();
}

//---Simpifying-------------------------------------------------------------------------------------------

void MeshViewer::MeshViewer::simplify_and_update() 
{
    if (!selected_object_) 
    {
        std::cerr << "No object selected" << std::endl;
        return;
    }
    
    const Mesh::Mesh& source_mesh = selected_object_->orig_mesh;
    Mesh::Mesh result;
    
    if (current_simplifier_index_ >= 0 && current_simplifier_index_ < simplifiers_.size()) 
    {
        try 
        {
            result = simplifiers_[current_simplifier_index_]->simplify(source_mesh, detail_level_);
            algo_name_ = simplifiers_[current_simplifier_index_]->get_name();
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Simplification failed: " << e.what() << std::endl;
            result = source_mesh;
            algo_name_ = "Error: " + std::string(e.what());
        }
    } 
    else 
    {
        result = source_mesh;
        algo_name_ = "None";
    }
    
    selected_object_->mesh = result;
    selected_object_->orig_mesh = source_mesh;
    selected_object_->detail_level = detail_level_;
    
    register_object(selected_object_);
    update_metrics();
}

//----UI drawing------------------------------------------------------------------------------------

void MeshViewer::MeshViewer::draw_simplification_ui() 
{
    if (simplifiers_.empty()) 
    {
        ImGui::TextColored(ImVec4(1,0,0,1), "No algorithms loaded!");
        ImGui::Text("Check console for errors");
        return;
    }
    
    // Кнопки выбора алгоритмов
    for (int i = 0; i < simplifiers_.size(); i++) 
    {
        if (ImGui::Button(simplifiers_[i]->get_name().c_str())) 
        {
            current_simplifier_index_ = i;
            algo_name_ = simplifiers_[i]->get_name();
            simplify_and_update();
            update_metrics();
        }
        if (i < simplifiers_.size() - 1) 
            ImGui::SameLine();
    }
    
    detail_level_slider();
    metrics_callback();
}

void MeshViewer::MeshViewer::detail_level_slider()
{
    static bool last_is_dragging = false; 
    static bool warned_about_speed = false;

    std::string current_algo_name = "Unknown";
    if (current_simplifier_index_ >= 0 && current_simplifier_index_ < simplifiers_.size()) 
    {
        current_algo_name = simplifiers_[current_simplifier_index_]->get_name();
    }
    
    bool is_slow_algo = (current_algo_name == "Edge Collapse" || current_algo_name == "Quadric");
    if (is_slow_algo && !warned_about_speed) 
    {
        ImGui::OpenPopup("Algo Warning");
        warned_about_speed = true;
    }
    
    if (ImGui::BeginPopupModal("Edge Collapse Warning")) 
    {
        ImGui::Text("This algo is significantly slower than Vertex Cluster.");
        ImGui::Text("It may take several seconds for large models.");
        ImGui::Text("The simplification will run when you release the slider.");
        if (ImGui::Button("OK, honey"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    
    bool changed = ImGui::SliderFloat("Share of simplification", &detail_level_, min_detail_, max_detail_, "%.3f");
    bool is_dragging = ImGui::IsItemActive();
    
    bool is_fast_algo = (current_algo_name == "Vertex Cluster");
    
    if (is_fast_algo) 
    {
        if (changed) 
        {
            simplify_and_update();
            update_metrics();
        }
    } 
    else 
    { 
        if (last_is_dragging && !is_dragging) 
        {
            simplify_and_update();
            update_metrics();
        }
        if (is_dragging)
            ImGui::TextColored(ImVec4(1,1,0,1), "Release slider to apply simplification");

        last_is_dragging = is_dragging;
    }
}

void MeshViewer::MeshViewer::draw_scene_ui() 
{
    if (ImGui::CollapsingHeader("Scene Objects", ImGuiTreeNodeFlags_DefaultOpen)) 
    {
        ImGui::BeginChild("ObjectList", ImVec2(200, 150), true);
        for (auto& obj : objects_) 
        {
            bool is_selected = (selected_object_ == obj.get());
            if (ImGui::Selectable(obj->name.c_str(), is_selected))
                selected_object_ = obj.get();
        }
        ImGui::EndChild();
        ImGui::SameLine();
        
        ImGui::BeginGroup();
        load_file_button();
        ImGui::Separator();
        add_button();
        remove_button();
        ImGui::Separator();
        ray_trace_button();
        ImGui::EndGroup();
        
        tune_properties();
    }
}

//---Cam control---------------------------------------------

void MeshViewer::MeshViewer::load_file_button()
{
    if (ImGui::Button("Load OBJ File"))
        ImGui::OpenPopup("Load Object Dialog");
    
    if (ImGui::BeginPopupModal("Load Object Dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
    {
        ImGui::Text("Load new 3D object");
        ImGui::Separator();
        
        ImGui::Text("File path:");
        ImGui::InputText("##filename", new_object_filename_, sizeof(new_object_filename_));
        
        ImGui::Text("Object name:");
        ImGui::InputText("##objname", new_object_name_, sizeof(new_object_name_));
        
        static float pos_x = 0.0f, pos_y = 0.0f, pos_z = 0.0f;
        ImGui::Text("Position:");
        ImGui::DragFloat("X##pos", &pos_x, 0.1f);
        ImGui::DragFloat("Y##pos", &pos_y, 0.1f);
        ImGui::DragFloat("Z##pos", &pos_z, 0.1f);
        
        static float color_r = 0.7f, color_g = 0.7f, color_b = 0.7f;
        ImGui::Text("Color:");
        ImGui::ColorEdit3("##newobjcolor", &color_r);
        
        ImGui::Separator();
        
        if (ImGui::Button("Load", ImVec2(120, 0))) 
        {
            std::string filename(new_object_filename_);
            std::string obj_name(new_object_name_);
            
            if (obj_name.empty()) 
            {
                size_t last_slash = filename.find_last_of("/\\");
                size_t last_dot = filename.find_last_of('.');
                if (last_slash != std::string::npos) last_slash++;
                else last_slash = 0;
                obj_name = filename.substr(last_slash, last_dot - last_slash);
            }
            
            load_object_from_file(filename, obj_name, Vec3f(pos_x, pos_y, pos_z), Vec3f(color_r, color_g, color_b));
            
            new_object_filename_[0] = '\0';
            new_object_name_[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void MeshViewer::MeshViewer::add_button()
{
    if (ImGui::Button("Add Copy"))
        ImGui::OpenPopup("Add Copy Dialog");
    
    if (ImGui::BeginPopupModal("Add Copy Dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
    {
        static char copy_name[256] = "";
        static float copy_pos_x = 2.0f, copy_pos_y = 0.0f, copy_pos_z = 0.0f;
        static float copy_color_r = 0.3f, copy_color_g = 0.7f, copy_color_b = 0.3f;
        
        ImGui::Text("Add copy of current model");
        ImGui::Separator();
        
        ImGui::Text("Name:");
        ImGui::InputText("##copyname", copy_name, sizeof(copy_name));
        
        ImGui::Text("Position:");
        ImGui::DragFloat("X##copy", &copy_pos_x, 0.1f);
        ImGui::DragFloat("Y##copy", &copy_pos_y, 0.1f);
        ImGui::DragFloat("Z##copy", &copy_pos_z, 0.1f);
        
        ImGui::Text("Color:");
        ImGui::ColorEdit3("##copycolor", &copy_color_r);
        
        ImGui::Separator();
        
        if (ImGui::Button("Add", ImVec2(120, 0))) 
        {
            auto new_obj = std::make_unique<SceneObject>();
            
            std::string obj_name(copy_name);
            if (obj_name.empty())
                obj_name = "copy_" + std::to_string(objects_.size());

            new_obj->name = obj_name;
            
            if (selected_object_) 
            {
                new_obj->orig_mesh = selected_object_->orig_mesh;
                new_obj->mesh = selected_object_->mesh;
                new_obj->detail_level = selected_object_->detail_level;
            } 
            else 
            {
                new_obj->orig_mesh = current_mesh_;
                new_obj->mesh = current_mesh_;
            }
            
            new_obj->position = Vec3f(copy_pos_x, copy_pos_y, copy_pos_z);
            new_obj->color = Vec3f(copy_color_r, copy_color_g, copy_color_b);
            new_obj->is_original = false;
            
            register_object(new_obj.get());
            objects_.push_back(std::move(new_obj));
            
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void MeshViewer::MeshViewer::remove_button()
{
    if (ImGui::Button("Remove Selected") && selected_object_ && !selected_object_->is_original) 
    {
        SceneObject* to_remove = selected_object_;
        auto it = std::find_if(objects_.begin(), objects_.end(), [to_remove](const auto& obj) { return obj.get() == to_remove; });
            
        if (it != objects_.end()) 
        {
            polyscope_object_delete(it->get());
            objects_.erase(it);
            
            // Выбираем следующий объект
            if (!objects_.empty()) 
            {
                // Сначала ищем оригинал
                auto orig_it = std::find_if(objects_.begin(), objects_.end(), [](const auto& obj) { return obj->is_original; });
                if (orig_it != objects_.end())
                    selected_object_ = orig_it->get();
                else
                    selected_object_ = objects_[0].get();
            } 
            else 
                selected_object_ = nullptr;
        }
    }
}

void MeshViewer::MeshViewer::ray_trace_button()
{
    if (ImGui::Button("Render Ray Traced", ImVec2(200, 0)))
        ImGui::OpenPopup("Render Settings");

    if (ImGui::BeginPopupModal("Render Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
    {
        static float light_pos_x = light_pos_.x(), light_pos_y = light_pos_.y(), light_pos_z = light_pos_.z();
        
        ImGui::Text("Render scene with specific parameters");
        ImGui::Separator();
        
        ImGui::Text("Output picture name:");
        ImGui::InputText("##picname", new_photo_name_, sizeof(new_photo_name_));
        
        ImGui::Text("Light position:");
        ImGui::DragFloat("X##light", &light_pos_x, 0.1f);
        ImGui::DragFloat("Y##light", &light_pos_y, 0.1f);
        ImGui::DragFloat("Z##light", &light_pos_z, 0.1f);
        
        ImGui::Separator();
        
        if (ImGui::Button("Render!", ImVec2(120, 0))) 
        {
            light_pos_ = Vec3f(light_pos_x, light_pos_y, light_pos_z);
            std::string photo_name(new_photo_name_);
            if (photo_name.empty()) 
                photo_name = "raytraced_output.png";
            
            render_raytraced(photo_name);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();
    
        ImGui::EndPopup();
    }
}

void MeshViewer::MeshViewer::tune_properties()
{
    if (selected_object_) 
    {
        ImGui::Separator();
        ImGui::Text("Properties: %s", selected_object_->name.c_str());
        
        // Цвет
        float color[3] = {selected_object_->color.x(), 
                          selected_object_->color.y(), 
                          selected_object_->color.z()};
        if (ImGui::ColorEdit3("Color", color)) 
        {
            selected_object_->color = Vec3f(color[0], color[1], color[2]);
            if (selected_object_->ps_handle)
                selected_object_->ps_handle->setSurfaceColor(glm::vec3(color[0], color[1], color[2]));
        }
        
        float pos[3] = {selected_object_->position.x(), selected_object_->position.y(), selected_object_->position.z()};
        if (ImGui::DragFloat3("Position", pos, 0.1f)) 
        {
            selected_object_->position = Vec3f(pos[0], pos[1], pos[2]);
            apply_transform(selected_object_);
        }
        
        bool visible = selected_object_->visible;
        if (ImGui::Checkbox("Visible", &visible)) 
        {
            selected_object_->visible = visible;
            if (selected_object_->ps_handle)
                selected_object_->ps_handle->setEnabled(visible);
        }
    }
}

//---Metrics-------------------------------------------------------------------------------

void MeshViewer::MeshViewer::metrics_callback() 
{
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 320, ImGui::GetIO().DisplaySize.y - 280), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, 260), ImGuiCond_Always);
    
    ImGui::Begin("Mesh Simplification Metrics", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Algorithm: %s", algo_name_.c_str());
    ImGui::Separator();

    ImGui::Text("Quality Metrics:");
    ImGui::Indent();

    ImGui::Text("Hausdorff distance:");
    ImGui::SameLine();
    ImVec4 hausdorff_color = get_color_for_metrics(current_hausdorff_, 0.1f, 1.0f);
    ImGui::TextColored(hausdorff_color, "%.2f%%", current_hausdorff_);
    
    ImGui::Text("RMS error:");
    ImGui::SameLine();
    ImVec4 rms_color = get_color_for_metrics(current_rms_, 0.05f, 0.5f);
    ImGui::TextColored(rms_color, "%.2f%%", current_rms_);
    ImGui::Separator();
    ImGui::End();
}

void MeshViewer::MeshViewer::register_object(SceneObject* obj) 
{
    if (!obj) return;
    
    std::string full_name = obj->name + "_" + std::to_string(reinterpret_cast<uintptr_t>(obj));
    
    if (obj->ps_handle)
        polyscope::removeSurfaceMesh(obj->ps_handle->name);
    
    if (obj->mesh.is_empty()) 
    {
        std::cerr << "Cannot register empty mesh: " << obj->name << std::endl;
        return;
    }
    
    obj->ps_handle = polyscope::registerSurfaceMesh(full_name, obj->mesh.get_vertices(), obj->mesh.get_triangles());
    
    obj->ps_handle->setSurfaceColor(glm::vec3(obj->color.x(), obj->color.y(), obj->color.z()));
    obj->ps_handle->setEnabled(obj->visible);
    apply_transform(obj);
}

void MeshViewer::MeshViewer::load_object_from_file(const std::string& filename, const std::string& name, const Vec3f& position, const Vec3f& color)
{
    try 
    {
        OBJParser::OBJParser parser(filename);
        Mesh::Mesh new_mesh(parser);
        
        auto new_obj = std::make_unique<SceneObject>();
        new_obj->name = name;
        new_obj->mesh = new_mesh;
        new_obj->orig_mesh = new_mesh;
        new_obj->position = position;
        new_obj->color = color;
        new_obj->is_original = false;
        new_obj->detail_level = 1.0f;
        
        register_object(new_obj.get());
        objects_.push_back(std::move(new_obj));
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Failed to load " << filename << ": " << e.what() << std::endl;
    }
}