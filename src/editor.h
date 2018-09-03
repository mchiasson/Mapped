#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include <string>

void editor_init();
void editor_new();
void editor_open();
void editor_openRecent(const std::string& filename);
void editor_save();
void editor_saveAs();
void editor_updateGUI();
void editor_quit();

#endif
