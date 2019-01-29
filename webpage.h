#ifndef WEBPAGE_H_
#define WEBPAGE_H_

#include <pgmspace.h>

const char PAGESTYLE[] PROGMEM = "<style>.c{text-align: center;} input[type=\"text\"]{width:45%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#34a3b4;color:#fff;line-height:2.4rem;font-size:1.2rem;width:50%;} .q{float: right;width: 64px;text-align: right;} </style>";

void pageinit();
void pageloop(void);

#endif //WEBPAGE_H_
