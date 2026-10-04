#ifndef SCRIB_COMPAT_H
#define SCRIB_COMPAT_H
#include <vitaGL.h>
#include <stdint.h>
int alex_uname(void *out);
int alex_sigprocmask(int how, const uint32_t *set, uint32_t *old);
void alex_exit(int status);
void alex_bind_renderbuffer(GLenum target, GLuint name);
void alex_renderbuffer_storage(GLenum target, GLenum format, GLsizei width, GLsizei height);
void alex_delete_renderbuffers(GLsizei count, const GLuint *names);
void alex_get_renderbuffer_parameter(GLenum target, GLenum pname, GLint *value);
#endif
