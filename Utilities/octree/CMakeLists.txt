set(template_classes
  octree/octree
  octree/octree_cursor
  octree/octree_iterator
  octree/octree_node
  octree/octree_path)

set(headers
  octree/octree)

vtk_module_add_module(VTK::octree
  TEMPLATE_CLASSES ${template_classes}
  HEADERS ${headers}
  HEADER_DIRECTORIES
  HEADER_ONLY
  EXCLUDE_HEADER_TEST)
