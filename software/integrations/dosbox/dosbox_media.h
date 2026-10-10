#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>

// Checked I/O only for inherited, leased raw images. Other files stay unchanged.
// Implemented in the maintained DOSBox dos.cpp integration.
void mister_dosbox_flush_media_files();
int mister_media_start();
int mister_media_finish();
bool mister_media_enabled();
void mister_media_track(FILE *stream);
// Return true only for an exact inherited writable lease descriptor path.
bool mister_media_open(const char *path, const char *mode, FILE **result);
size_t mister_media_write(const void *data, size_t size, size_t count, FILE *stream);
int mister_media_seek(FILE *stream, int64_t offset, int whence);
int mister_media_close(FILE *stream);
