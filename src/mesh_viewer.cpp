#include "mesh_viewer.hpp"

void MeshViewer::MeshViewer::show_mesh() 
{
    polyscope::init();

    auto original_obj = std::make_unique<SceneObject>();
    original_obj->name = "original";
    original_obj->mesh = orig_mesh_;
    original_obj->orig_mesh = orig_mesh_;
    original_obj->is_original = true;
    original_obj->color = Vec3::Vec3f(0.8f, 0.8f, 0.8f);

    register_object(original_obj.get());
    selected_object_ = original_obj.get();
    objects_.push_back(std::move(original_obj));

    polyscope::state::userCallback = [this]() {
        this->draw_ui();
    };

    polyscope::show();
}

void MeshViewer::MeshViewer::update_detail_range() 
{
    int orig_faces = orig_mesh_.get_triang_amt();
    
    min_detail_ = 0.0f;

    int target_faces = 3;
    max_detail_ = 1.0f - (float)target_faces / orig_faces - 0.5f;

    if (detail_level_ > max_detail_)
        detail_level_ = max_detail_;
    if (detail_level_ < min_detail_)
        detail_level_ = min_detail_;
}

void MeshViewer::MeshViewer::render_raytraced() 
{
    glm::vec3 cam_global_pos = polyscope::view::getCameraWorldPosition();
    glm::mat4 view_matrix = polyscope::view::getCameraViewMatrix();
    glm::mat4 inverted_view_matrix = glm::inverse(view_matrix);
        
    glm::vec3 forward = glm::normalize(glm::vec3(inverted_view_matrix[2]));
    glm::vec3 up = glm::normalize(glm::vec3(inverted_view_matrix[1]));
    glm::vec3 right = glm::normalize(glm::cross(forward, up));

    float min_x = 1e30f, max_x = -1e30f;
    float min_y = 1e30f, max_y = -1e30f;
    float min_z = 1e30f, max_z = -1e30f;
        
    for (const auto& obj : objects_) 
    {
        auto bounding_box = obj->mesh.get_bounding_box();

        min_x = std::min(min_x, bounding_box[0].x());
        max_x = std::max(max_x, bounding_box[1].x());
        min_y = std::min(min_y, bounding_box[0].y());
        max_y = std::max(max_y, bounding_box[1].y());
        min_z = std::min(min_z, bounding_box[0].z());
        max_z = std::max(max_z, bounding_box[1].z());
    }

    Vec3f model_center((min_x + max_x) / 2, (min_y + max_y) / 2, (min_z + max_z) / 2);
    glm::vec3 target(model_center.x(), model_center.y(), model_center.z());

    float distance = glm::distance(cam_global_pos, target);
    
    Vec3f camera_pos(cam_global_pos.x, cam_global_pos.y, cam_global_pos.z);
    Vec3f camera_target(target.x, target.y, target.z);
    Vec3f camera_up(up.x, up.y, up.z);
    RayTracer::Settings settings;
    settings.camera_position = camera_pos;
    settings.camera_target = camera_target;
    settings.camera_up = camera_up;
    settings.vertical_fov_deg = 45.0;
    settings.width = 800;
    settings.height = 600;
    settings.light_pos = Vec3f(5.0f, 10.0f, 5.0f);          //!! может, добавить в юай возможность менять свет?
    
    // через shared_ptr безопасно для потока
    auto rt = std::make_shared<RayTracer::RayTracer>(settings);
    //к каждому объекту к их локальным координатам надо добавить позицию в мировых координатах, тогда объекты не будут перекрываться
    for (const auto& obj : objects_) 
    {
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
    //может, добавить, чтобы задавать имя этого файла
    std::thread render_thread([rt]() 
    {
        rt->render("raytraced_output.png");
    });

    render_thread.detach();
}

void MeshViewer::MeshViewer::simplify_and_update() 
{
    if (!selected_object_) return;
    Mesh::Mesh result;
    
    const Mesh::Mesh& source_mesh = selected_object_->orig_mesh;
    switch(algo_) 
    {
        case VERTEX_CLUSTER: 
        {
            algo_name_ = "Vertex cluster";
            VertexCluster::VertexCluster simplifier(source_mesh, detail_level_);
            result = simplifier.simplify();
            break;
        }
        case EDGE_COLLAPSE: 
        {
            algo_name_ = "Edge collapse";
            Mesh::EdgeMesh edge_mesh(source_mesh);
            EdgeCollapse::SimpleEdgeCollapse simplifier(edge_mesh, (1 - detail_level_));
            result = simplifier.simplify();
            break;
        }
        case QUADRIC: 
        {
            algo_name_ = "Quadric";
            Mesh::EdgeMesh edge_mesh(source_mesh);
            EdgeCollapse::QuadricEdgeCollapse simplifier(edge_mesh, (1 - detail_level_));
            result = simplifier.simplify();
            break;
        }
        default:
            result = source_mesh;
    }
    // Обновляем mesh выбранного объекта
    selected_object_->mesh = result;
    selected_object_->orig_mesh = source_mesh;
    selected_object_->detail_level = detail_level_;
    selected_object_->current_algo = algo_;
    // Перерегистрируем объект в Polyscope
    register_object(selected_object_);
    update_metrics();
}

void MeshViewer::MeshViewer::draw_simplification_ui() 
{
    static bool warned_about_edge_collapse = false;
    static bool was_dragging = false;
    static bool last_is_dragging = false; 
    if (ImGui::Button("Vertex Cluster")) 
    {
        algo_ = VERTEX_CLUSTER;
        simplify_and_update();
        update_metrics();
    }
    ImGui::SameLine();
    if (ImGui::Button("Edge Collapse")) 
    {
        algo_ = EDGE_COLLAPSE;
        simplify_and_update();
        update_metrics();
    }
    ImGui::SameLine();
    if (ImGui::Button("Quadric")) 
    {
        algo_ = QUADRIC;
        simplify_and_update();
        update_metrics();
    }
    if (algo_ == EDGE_COLLAPSE && !warned_about_edge_collapse) 
    {
        ImGui::OpenPopup("Edge Collapse Warning");
        warned_about_edge_collapse = true;
    }
    if (ImGui::BeginPopupModal("Edge Collapse Warning")) 
    {
        ImGui::Text("Edge Collapse is significantly slower than Vertex Cluster.");
        ImGui::Text("It may take several seconds for large models.");
        ImGui::Text("The simplification will run when you release the slider.");
        if (ImGui::Button("OK, honey"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    bool changed = ImGui::SliderFloat("Share of simplification", &detail_level_, min_detail_, max_detail_, "%.3f");
    bool is_dragging = ImGui::IsItemActive();
    if (algo_ == VERTEX_CLUSTER || algo_ == QUADRIC) 
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
            ImGui::TextColored(ImVec4(1,1,0,1), "Release slider to apply edge collapse and wait (please, sir)");
        last_is_dragging = is_dragging;
    }
    metrics_callback();
}

//поделить 
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
        if (ImGui::Button("Add Current Simplified")) 
            ImGui::OpenPopup("Add Simplified Object");
        if (ImGui::BeginPopupModal("Add Simplified Object", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
        {
            static float pos_x = 2.0f, pos_y = 0.0f, pos_z = 0.0f;
            static float color_r = 0.3f, color_g = 0.7f, color_b = 0.3f;
            ImGui::Text("Position:");
            ImGui::DragFloat("X", &pos_x, 0.1f);
            ImGui::DragFloat("Y", &pos_y, 0.1f);
            ImGui::DragFloat("Z", &pos_z, 0.1f);
            ImGui::Text("Color:");
            ImGui::ColorEdit3("Color", &color_r);
            ImGui::Separator();
        
            if (ImGui::Button("Add", ImVec2(120, 0))) 
            {
                auto new_obj = std::make_unique<SceneObject>();
                new_obj->name = "simplified_" + std::to_string(objects_.size());
                if (selected_object_) 
                {
                    new_obj->orig_mesh = selected_object_->orig_mesh;
                    new_obj->mesh = selected_object_->orig_mesh;      //не уверена. ползунок ведь будет на начале. это странно
                    new_obj->detail_level = 0.0f;
                    new_obj->current_algo = algo_;
                } else {
                    new_obj->orig_mesh = current_mesh_;
                    new_obj->mesh = current_mesh_;
                }
                new_obj->position = Vec3f(pos_x, pos_y, pos_z);
                new_obj->color = Vec3f(color_r, color_g, color_b);
                new_obj->is_original = false;
                register_object(new_obj.get());
                objects_.push_back(std::move(new_obj));
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) 
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        if (ImGui::Button("Remove Selected") && selected_object_ && !selected_object_->is_original) 
        {
            SceneObject* to_remove = selected_object_;
            auto it = std::find_if(objects_.begin(), objects_.end(), [to_remove](const auto& obj) { return obj.get() == to_remove; });
            if (it != objects_.end()) 
            {
                polyscope_object_delete(it->get());
                objects_.erase(it);
                if (!objects_.empty()) 
                {
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
        ImGui::Separator();
        if (ImGui::Button("Render Ray Traced", ImVec2(200, 0)))
            render_raytraced();
        ImGui::EndGroup();
        if (selected_object_) 
        {
            ImGui::Separator();
            ImGui::Text("Properties: %s", selected_object_->name.c_str());
            float color[3] = {selected_object_->color.x(), selected_object_->color.y(), selected_object_->color.z()};
            if (ImGui::ColorEdit3("Color", color)) 
            {
                selected_object_->color = Vec3::Vec3f(color[0], color[1], color[2]);
                if (selected_object_->ps_handle)
                    selected_object_->ps_handle->setSurfaceColor(glm::vec3(color[0], color[1], color[2]));
            }
            float pos[3] = {selected_object_->position.x(), selected_object_->position.y(), selected_object_->position.z()};
            if (ImGui::DragFloat3("Position", pos, 0.1f)) 
            {
                selected_object_->position = Vec3::Vec3f(pos[0], pos[1], pos[2]);
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
}


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

    ImVec4 hausdorff_color = get_color_for_metrics(current_hausdorff_, 0.1, 1);
    ImGui::TextColored(hausdorff_color, "%.2f", current_hausdorff_);
    ImGui::Text("RMS error:");
    ImGui::SameLine();

    ImVec4 rms_color = get_color_for_metrics(current_rms_, 0.05, 0.5);
    ImGui::TextColored(rms_color, "%.2f", current_rms_);
    ImGui::Separator();

    ImGui::End();
}
    
void MeshViewer::MeshViewer::register_object(SceneObject* obj) 
{
    std::string full_name = obj->name + "_" + std::to_string(reinterpret_cast<uintptr_t>(obj));
    
    if (obj->ps_handle)
        polyscope::removeSurfaceMesh(obj->ps_handle->name);
    
    obj->ps_handle = polyscope::registerSurfaceMesh(full_name, obj->mesh.get_vertices(), obj->mesh.get_triangles());
    
    obj->ps_handle->setSurfaceColor(glm::vec3(obj->color.x(), obj->color.y(), obj->color.z()));
    obj->ps_handle->setEnabled(obj->visible);
    
    apply_transform(obj);
}