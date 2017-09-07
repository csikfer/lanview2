#ifndef LINKGRAPH_H
#define LINKGRAPH_H
#include "lv2g.h"
#ifdef ZODIACGRAPH
#include <node.h>
#endif

class cLinkGraph : public cIntSubObj
{
public:
    cLinkGraph(QMdiArea *par);
};

#endif // LINKGRAPH_H
