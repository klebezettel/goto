#ifndef IMODEL_H
#define IMODEL_H

#include "imenuitem.h"

namespace Core {

class IModel
{
public:
    virtual MenuItems items(bool refresh) = 0;
};

} // namespace Core

#endif // IMODEL_H
