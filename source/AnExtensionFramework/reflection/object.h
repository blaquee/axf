#ifndef object_h__
#define object_h__

#include <type_traits>
#include <string>
#include <vector>
#include <list>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

/*
    This File implements the Object Registry which stores reflection information about the available objects

    Every TypeInstance in this registry has a mapping from type_id -> module.path.name, also known as fully qualified name

    Every type_id/module.path points to a TypeInstance object which describes the structure itself

    A TypeInstance object itself is a collection of Types, to demonstrate look at the following examples

    TypeInstance Dog
    {
        TypeInstance<int> Eyes;
        TypeInstance<Enum<Color>> Color;
        TypeInstance<string> Name;

        TypeInstance<Func> Create;
        TypeInstance<Func> Destroy;
        TypeInstance<Func> Bark;
    }

    TypeInstance TypeInstance<int>
    {
        TypeInstance<Func> Get;
        TypeInstance<Func> Set;
    }

    TypeInstance TypeInstance<string>
    {
        TypeInstance<Func> Get;
        TypeInstance<Func> Set;
    }

    TypeInstance TypeInstance<Func>
    {
        Enum<FuncType> FuncType; // static, member, class
        TypeInstance<Pointer> FuncPointer;
        TypeInstance<Tuple<TypeInstance>> Params;
    }

    A special root TypeInstance points to the very top level of the module hierarchy may be used to start off the enumeration

    TypeInstance<Node> root = GetRoot();
    TypeInstance TypeInstance<Node>
    {
        Enum<NodeType> NodeType;
        TypeInstance<String> Name;
        vector<TypeInstance> Types;
    }

    A generic visitor can be used on a TypeInstance to perform enumerations

    Types can be registered at compile time or dynamically during runtime by using the macros/apis found in reflection.h

    Summary: 
        The purpose of this reflection is to construct a tree structure of every Types that is provided by the programmer.
        The reflection information can then be further processed to provide utilities to script binding, serialization, etc
*/

struct NodeType
{
    enum Type
    {
        COLLECTION, // Collection types either MODULE, CLASS, INTERFACE or MAP

        //member type may either be a Function, a Variable or a Mapping
        FUNCTION, VARIABLE, OPERATOR,
        GETSET
    };
};

struct MemberType
{
    enum Type
    {
        STATIC, MEMBER, CLASS
    };
};

struct CollectionType
{
    enum Type
    {
        MODULE, // aka namespace in c++
        CLASS, INTERFACE, 
        MAP // array, dict, set, list, etc
    };
};

// A 32bit ID based TypeInstance
class TypeInstance32
{
public:
    typedef std::uint32_t TypeID;
    typedef TypeID TypeInstanceID;

    bool isDeleted; // true if this type has been deleted

    // the node (namespace) this type belongs to
    TypeInstanceID parentID;

    TypeInstanceID instanceID; //an id that uniquely identifies this instance
    TypeID typeID; // a unique system-wide id, switch to 64bit when using hash, An id of 0 (Zero) is an unregistered TypeInstance!
    std::string name;

    std::unordered_set<TypeInstanceID> childNodes;

    union
    {
        struct
        {
            // if nodeType ==  VARIABLE
            MemberType::Type memberType;

            void*(VarPtr)(void *obj); // get pointer to variable for fast read/write
            void(GetVar)(void *obj, void *ret);
            void(SetVar)(void *obj, void *val);
        } member;

        struct
        {
            // if nodeType == COLLECTION
            MemberType::Type memberType; 
            CollectionType::Type collectionType;
        } collection;

        struct 
        {
            // if nodeType ==  FUNCTION or OPERATOR
            MemberType::Type memberType;

            // storage for the member function pointer
            // the buffer should be big enough, use compile time check to ensure
            char ptr[20]; 
        } function;

        struct 
        {
            // if nodeType ==  GETSET
            MemberType::Type memberType;
            bool readonly;

            // storage for the member function pointer
            // the buffer should be big enough, use compile time check to ensure
            char ptrGet[20]; 
            char ptrSet[20]; 
        } getset;
    };


    TypeInstance32(TypeInstanceID parentID, TypeInstanceID instanceID, TypeID typeID,  const std::string &name,
                   const std::unordered_set<TypeInstanceID> &childNodes = std::unordered_set<TypeInstanceID>());

    inline bool operator ==(const TypeInstance32 &right) const { return instanceID == right.instanceID; }
    inline bool IsSameType(const TypeInstance32 &right) const { return typeID == right.typeID; }
};

// Starting from here, we'll use the typename TypeInstance instead of TypeInstance32
typedef TypeInstance32 TypeInstance;


// Contains the actual TypeInstance information
class TypeClass
{
public:
    bool isDeleted; // true if this type has been deleted

    TypeInstance::TypeID ownerID; // the owner of this type class, must always be a valid id
    TypeInstance::TypeID typeID; // the id of this type class, always valid
    
    std::string name; //name of this type
    NodeType::Type nodeType;

    std::vector<TypeInstance::TypeID> parents; // the parents (inheritance) of this type, if any

    TypeClass(TypeInstance::TypeID ownerID, TypeInstance::TypeID typeID, NodeType::Type nodeType, const std::string &name, 
              const std::vector<TypeInstance::TypeID> &parents = std::vector<TypeInstance::TypeID>());
};

// The type registry stores a flat dict that maps from type_id -> type_storage and full_type_name -> type_storage
// When adding/removing a TypeInstance be sure to update both the type_id and full_type_name dicts
// TypeInstance ids are allocated sequentially, so a vector is used instead for extremely fast lookup
// TypeInstance names however must require a dictionary lookup which is very slow
// if a type is deleted a null is returned when doing a lookup
class TypeRegistry
{
public:
    // when removing types/instances just set the isDeleted member to false, 
    // do not remove the instance object from the vector because it ruin the ordering
    std::vector<TypeInstance> typeInstancesByID; // TypeInstanceID -> TypeInstance
    std::vector<TypeClass> typesByID;  // TypeID -> TypeClass
    std::unordered_map<std::string, TypeInstance::TypeID> typesByFullname; 

    TypeRegistry();

    TypeInstance::TypeID AddType(TypeInstance::TypeID ownerID, NodeType::Type type, const std::string &name);
    TypeInstance::TypeInstanceID AddTypeInstance(TypeInstance::TypeInstanceID parentID, TypeInstance::TypeID typeID, const std::string &name);
};

#endif // object_h__

