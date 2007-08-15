function render_frame(f, t)
	--local surf = cairo.image_surface_create(200,200,"argb32")
	local surf = f.create_cairo_surface()
	local ctx = surf.create_context()
	
	ctx.set_source_rgba(1, 0.5, 0.5, 0.75)
	ctx.move_to(10, 10)
	ctx.line_to(10, 200)
	ctx.line_to(300, 10)
	ctx.close_path()
	ctx.fill_preserve()
	ctx.set_source_rgba(0,0,0,1)
	ctx.stroke()
	
	ctx.push_group()
	ctx.select_font_face("Arial", "italic", "")
	ctx.set_font_size(35)
	ctx.move_to(100,100)
	ctx.text_path(string.format("Time: %.3fs", t))
	ctx.set_source_rgba(0,1,0,0.8)
	ctx.set_line_width(4)
	ctx.stroke_preserve()
	ctx.set_source_rgba(0,0,1,1)
	ctx.fill()
	ctx.pop_group_to_source()
	ctx.paint_with_alpha(0.5)
	
	f.overlay_cairo_surface(surf, 0, 0)
	
	surf = cairo.image_surface_create(400, 200, "argb32")
	ctx = surf.create_context()
	ctx.select_font_face("Arial", "", "bold")
	ctx.set_source_rgba(1, 1, 1, 1)
	ctx.set_font_size(40)
	ctx.move_to(100, 100)
	ctx.text_path("OverLua")
	ctx.fill()
	
	raster.directional_blur(surf, t, 5)
	
	f.overlay_cairo_surface(surf, 200, 50)
end
