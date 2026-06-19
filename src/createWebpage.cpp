//createWebpage.cpp
#include "../include/webPage.hpp"

int main()
{
    WebPage *w = new WebPage("../corpus/人民网语料");
    w->buildPage("../data/newripepage.dat");
    delete w;
    return 0;
}