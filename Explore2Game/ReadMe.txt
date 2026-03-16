
Planet construction:
sub-divided cube n x n per side
m x m blocks per side with a texture assigned to each

Each quad on the planet is added to a list.
Each quad is the root of a chunked lod.

For each chunked lod l
	evaluate_lod(l, camera, err_tolerance)
end for

evaluate_lod(l, camera, err_tolerance)
{
	smallest_d = inf
	for each pt p in l.boundingbox 
	{
		d = length(camera.pos - p)
		if d < smallest_d
			smallest_d = d
	}
	err = screen_space_err(d, l.geometric_error)
	if err > err_tolerance
	{
		l.visible = false
		split(l)
	}
	else
		l.visible = true
}

