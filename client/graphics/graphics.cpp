

#if 0
static void draw_requests_to_vertices(const Draw_Request_Circle* rs, Vertex* vs, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        const auto& r = rs[i];
        auto circle = &vs[i * rs->num_vertices];
        auto center = r.position;

        auto side_count = r.num_vertices / 3.0;
        auto degree_per_triangle = 360.0 / side_count;

        Array<Vec2> circle_points;
        for (size_t j = 0; j < side_count; j++) {
            auto point = Vec2(center.x + r.radius * cos(degree_per_triangle * j * M_PI / 180.0),
                center.y + r.radius * sin(degree_per_triangle * j * M_PI / 180.0));
            circle_points.emplace_back(point);
        }

        for (size_t j = 0; j < side_count; j++) {
            auto tris = &circle[j * 3];
            
            tris[0].position = center;
            tris[0].color = r.color;

            tris[1].position = circle_points[j];
            tris[1].color = r.color;


            if (j != side_count - 1)
                tris[2].position = circle_points[j + 1];
            else
                tris[2].position = circle_points[0];
            tris[2].color = r.color;

        }

    }
}
#endif
