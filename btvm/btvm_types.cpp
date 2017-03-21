#include "btvm_types.h"
#include <string>

/*
*************** 010 Editor type definitions ***************

struct TFindResults { uint64_t count; uint64_t start[]; uint64_t size[]; };
*/

/* *********************************************************** */
/* ********************** Type Creation ********************** */
/* *********************************************************** */

#define identifier(id) (new NIdentifier(id))
#define empty_block() (new NBlock())

static NScalarType* buildScalar(const std::string& name, uint64_t bits, bool issigned)
{
    NScalarType* nscalar = new NScalarType(name, bits);
    nscalar->is_signed = issigned;
    return nscalar;
}

static NVariable* buildVariable(const std::string& name, Node* ntype, Node* nsize = NULL)
{
    NVariable* nvar = new NVariable(identifier(name));
    nvar->type = ntype;
    nvar->size = nsize;
    return nvar;
}

static NStruct* buildStruct(const std::string& name, const NodeList& members) { return new NStruct(identifier(name), NodeList(), members); }

/* ******************************************************* */
/* ********************** Built-ins ********************** */
/* ******************************************************* */

Node *BTVMTypes::buildTFindResults()
{
    NodeList members;
    members.push_back(buildVariable("count", buildScalar("uint64", 32, false)));
    members.push_back(buildVariable("start", buildScalar("uint64", 32, false), empty_block()));
    members.push_back(buildVariable("size", buildScalar("uint64", 32, false), empty_block()));

    return buildStruct("TFindResults", members);
}
