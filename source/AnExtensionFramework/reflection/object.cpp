#include "object.h"
#include <sstream>

TypeInstance32::TypeInstance32(TypeInstanceID parentID, TypeInstanceID instanceID, TypeID typeID, 
                               const std::string &name,
                               const std::unordered_set<TypeInstanceID> &childNodes)
    : isDeleted(false), parentID(parentID), instanceID(instanceID), typeID(typeID), name(name), childNodes(childNodes)
{
    /*std::ostringstream ss;
    for(std::vector<std::string>::size_type i = 0; i < module->size(); ++i)
    {
        if(i != 0)
            ss << ",";
        ss << (*module)[i];
    }

    fullname = ss.str();*/
}


TypeClass::TypeClass(TypeInstance::TypeID ownerID, TypeInstance::TypeID typeID, NodeType::Type nodeType, const std::string &name, 
                     const std::vector<TypeInstance::TypeID> &parents)
    : isDeleted(false),  ownerID(ownerID), typeID(typeID), nodeType(nodeType), name(name), parents(parents)
{

}

TypeRegistry::TypeRegistry()
{

}

TypeInstance::TypeID TypeRegistry::AddType(TypeInstance::TypeID ownerID, NodeType::Type type, const std::string &name)
{
    TypeInstance::TypeID typeID = static_cast<TypeInstance::TypeID>(typesByID.size());
    typesByID.push_back(TypeClass(ownerID, typeID, type, name));
    typesByFullname[name] = typeID;
}

TypeInstance::TypeInstanceID TypeRegistry::AddTypeInstance(TypeInstance::TypeInstanceID parentID, TypeInstance::TypeID typeID, const std::string &name )
{
    TypeInstance::TypeInstanceID instanceID = static_cast<TypeInstance::TypeInstanceID>(typeInstancesByID.size());
    typeInstancesByID.push_back(TypeInstance(parentID, instanceID, typeID, name));
}
