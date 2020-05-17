#ifndef SHADER_HPP
#define SHADER_HPP
GLuint CompileShaders(const char * VertexSourcePointer, const char * FragmentSourcePointer);
GLuint CompileShaders(const char * VertexSourcePointer, const char * FragmentSourcePointer, const char * GeometrySource);

GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path);

#endif
