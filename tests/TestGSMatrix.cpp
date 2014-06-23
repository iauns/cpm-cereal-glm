
#include <entity-system/GenericSystem.hpp>
#include <entity-system/ESCore.hpp>
#include <es-cereal/CerealCore.hpp>
#include <cereal-glm/CerealGLM.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace es = CPM_ES_NS;
namespace cereal = CPM_ES_CEREAL_NS;

namespace {

// We may want to enforce that these components have bson serialization members
// (possibly a static assert?).

struct CompMat3
{
  CompMat3() {}
  CompMat3(const glm::mat3& rot) {rotation = rot;}

  void checkEqual(const CompMat3& rot_in) const
  {
    glm::mat3 rot = rot_in.rotation;
    EXPECT_FLOAT_EQ(rotation[0][0], rot[0][0]); EXPECT_FLOAT_EQ(rotation[0][1], rot[0][1]); EXPECT_FLOAT_EQ(rotation[0][2], rot[0][2]);
    EXPECT_FLOAT_EQ(rotation[1][0], rot[1][0]); EXPECT_FLOAT_EQ(rotation[1][1], rot[1][1]); EXPECT_FLOAT_EQ(rotation[1][2], rot[1][2]);
    EXPECT_FLOAT_EQ(rotation[2][0], rot[2][0]); EXPECT_FLOAT_EQ(rotation[2][1], rot[2][1]); EXPECT_FLOAT_EQ(rotation[2][2], rot[2][2]);
  }

  // What this 'struct' is all about -- the data.
  glm::mat3 rotation;

  static const char* getName() {return "ren:mat3";}

  static int SerializeCalls;
  bool serialize(cereal::ComponentSerialize& s, uint64_t /* entityID */)
  {
    ++SerializeCalls;
    s.serialize("m3", rotation);
    return true;
  }
};

int CompMat3::SerializeCalls = 0;

struct CompMat4
{
  CompMat4() {}
  CompMat4(const glm::mat4& trafo) {transform = trafo;}

  void checkEqual(const CompMat4& trafo_in) const
  {
    glm::mat4 trafo = trafo_in.transform;
    EXPECT_FLOAT_EQ(transform[0][0], trafo[0][0]); EXPECT_FLOAT_EQ(transform[0][1], trafo[0][1]); EXPECT_FLOAT_EQ(transform[0][2], trafo[0][2]); EXPECT_FLOAT_EQ(transform[0][3], trafo[0][3]);
    EXPECT_FLOAT_EQ(transform[1][0], trafo[1][0]); EXPECT_FLOAT_EQ(transform[1][1], trafo[1][1]); EXPECT_FLOAT_EQ(transform[1][2], trafo[1][2]); EXPECT_FLOAT_EQ(transform[1][3], trafo[1][3]);
    EXPECT_FLOAT_EQ(transform[2][0], trafo[2][0]); EXPECT_FLOAT_EQ(transform[2][1], trafo[2][1]); EXPECT_FLOAT_EQ(transform[2][2], trafo[2][2]); EXPECT_FLOAT_EQ(transform[2][3], trafo[2][3]);
    EXPECT_FLOAT_EQ(transform[3][0], trafo[3][0]); EXPECT_FLOAT_EQ(transform[3][1], trafo[3][1]); EXPECT_FLOAT_EQ(transform[3][2], trafo[3][2]); EXPECT_FLOAT_EQ(transform[3][3], trafo[3][3]);
  }

  // DATA
  glm::mat4 transform;

  static const char* getName() {return "ren:mat4";}

  static int SerializeCalls;
  bool serialize(cereal::ComponentSerialize& s, uint64_t /* entityID */)
  {
    ++SerializeCalls;
    s.serialize("m4", transform);
    return true;
  }
};
int CompMat4::SerializeCalls = 0;

float mat3_a[12] = {
  1.3f, 9.82, 53.9,
  -9.32, -293.0, 234.0,
  32.0, 12.4, -1.35
};

float mat3_b[12] = {
  5.3f, -9.82, 0.9,
  -0.32, 93.0, 921.0,
  32.285, 12.294, -324.35
};

float mat3_c[12] = {
  -982.32f, 0.35, 0.3245,
  910.32, 293.0, 1.0,
  325.0, 981.53, -325.82
};

float mat4_a[16] = {
  92.87, -291.82, -32.352, 8888.0,
  23.32, 8934.0, 1651.0, -13.0,
  99.582, 221.4, -873.32, 8792.38,
  3252.582, 22.4, -8.22, -8982.299
};

float mat4_b[16] = {
  5.3f, -9.82, 0.9, 88325.0,
  -0.32, 93.0, 921.0, 908.0,
  32.285, 12.294, -324.35, 759.0,
  982.52, 29890.0, 648379.0, -867534.0
};

float mat4_c[16] = {
  23.32, 8934.0, 1651.0, -13.0,
  5.3f, -9.82, 0.9, 88325.0,
  -0.32, 93.0, 921.0, 908.0,
  99.582, 221.4, -873.32, 8792.38,
};

// Component positions. associated with id. The first component is not used.
std::vector<CompMat3> mat3Components = {
  glm::mat3(glm::make_mat3(mat3_a)),
  glm::mat3(glm::make_mat3(mat3_a)),
  glm::mat3(glm::make_mat3(mat3_b)),
  glm::mat3(glm::make_mat3(mat3_c))
};

std::vector<CompMat4> mat4Components = {
  glm::mat4(glm::make_mat4(mat4_a)),
  glm::mat4(glm::make_mat4(mat4_a)),
  glm::mat4(glm::make_mat4(mat4_b)),
  glm::mat4(glm::make_mat4(mat4_c))
};

class BasicSystem : public es::GenericSystem<false, CompMat3, CompMat4>
{
public:

  static std::map<uint64_t, bool> invalidComponents;

  void execute(es::ESCoreBase&, uint64_t entityID,
               const CompMat3* mat3, const CompMat4* mat4) override
  {
    // Check to see if this entityID should have been executed.
    if (invalidComponents.find(entityID) != invalidComponents.end())
      FAIL() << "BasicSystem attempt to execute on an invalid component." << std::endl;

    // Check the values contained in each of pos, homPos, and gp.
    mat3->checkEqual(mat3Components[entityID]);
    mat4->checkEqual(mat4Components[entityID]);
  }

  // Compile time polymorphic function required by CerealCore when registering.
  static const char* getName()
  {
    return "ren:BasicSystem";
  }
};

class SystemOne : public es::GenericSystem<false, CompMat4>
{
public:
  static std::map<uint64_t, bool> invalidComponents;

  void execute(es::ESCoreBase&, uint64_t entityID, const CompMat4* mat4) override
  {
    // Check to see if this entityID should have been executed.
    if (invalidComponents.find(entityID) != invalidComponents.end())
      FAIL() << "SystemOne attempt to execute on an invalid component." << std::endl;

    // Check the values contained in each of pos, homPos, and gp.
    mat4->checkEqual(mat4Components[entityID]);
  }
  
  // Compile time polymorphic function required by CerealCore when registering.
  static const char* getName()
  {
    return "ren:SystemOne";
  }
};

std::map<uint64_t, bool> BasicSystem::invalidComponents;
std::map<uint64_t, bool> SystemOne::invalidComponents;

TEST(EntitySystem, BasicConstruction)
{
  // Generate entity system core.
  std::shared_ptr<cereal::CerealCore> core(new cereal::CerealCore());

  // Register components. Registration validates component names and ensures
  // that all components are present when serialization is performed.
  // Registration is not mandatory, but deserialization will fail if all
  // components and systems have not been executed.
  core->registerComponent<CompMat3>();
  core->registerComponent<CompMat4>();

  // Destroy the core and re-register components.
  // Then deserialize the data from memory to see if the correct components
  // and systems are serialized back in.

  uint64_t rootID = core->getNewEntityID();
  uint64_t id = rootID;
  core->addComponent(id, mat3Components[id]);
  core->addComponent(id, mat4Components[id]);

  id = core->getNewEntityID();
  core->addComponent(id, mat3Components[id]);
  BasicSystem::invalidComponents.insert(std::make_pair(id, true));

  id = core->getNewEntityID();
  core->addComponent(id, mat3Components[id]);
  core->addComponent(id, mat4Components[id]);

  std::shared_ptr<BasicSystem> sysBasic(new BasicSystem());
  std::shared_ptr<SystemOne> sysOne(new SystemOne());

  core->renormalize(true);
  sysBasic->walkComponents(*core);
  sysOne->walkComponents(*core);

  Tny* root = core->serializeAllComponents();

  // Check the structure of the document to ensure all data was serialized.
  // Data should be serialized in the order of their template ID. Since
  // we registered the components above before using, they should be in the
  // registration order.
  ASSERT_TRUE(CPM_ES_NS::TemplateID<CompMat3>::getID() < CPM_ES_NS::TemplateID<CompMat4>::getID());

  auto checkCompM3 = [](uint64_t compID, Tny* obj) -> Tny*
  {
    EXPECT_EQ(TNY_INT64, obj->type);
    EXPECT_EQ(compID, obj->value.num);
    EXPECT_EQ(1, Tny_hasNext(obj));
    obj = Tny_next(obj);
    EXPECT_EQ(TNY_OBJ, obj->type);

    glm::mat3 data;
    {
      Tny* dictRoot = obj->value.tny;
      Tny* comp = dictRoot;
      EXPECT_EQ(TNY_DICT, comp->type);
      EXPECT_EQ(1, Tny_hasNext(comp));

      comp = Tny_next(comp);
      EXPECT_EQ(TNY_OBJ, comp->type); // There is no float type. Swap is the same for INT32.
      EXPECT_EQ("m3", std::string(comp->key));
      cereal::CerealSerializeType<glm::mat3>::in(dictRoot, "m3", data);  // Will lookup the value in dictRoot.

      CompMat3 compareComp(data);
      compareComp.checkEqual(mat3Components[compID]);
    }

    return obj;
  };

  auto checkCompM4 = [](uint64_t compID, Tny* obj) -> Tny*
  {
    EXPECT_EQ(TNY_INT64, obj->type);
    EXPECT_EQ(compID, obj->value.num);
    EXPECT_EQ(1, Tny_hasNext(obj));
    obj = Tny_next(obj);
    EXPECT_EQ(TNY_OBJ, obj->type);

    glm::mat4 data;
    {
      Tny* dictRoot = obj->value.tny;
      Tny* comp = dictRoot;
      EXPECT_EQ(TNY_DICT, comp->type);
      EXPECT_EQ(1, Tny_hasNext(comp));

      comp = Tny_next(comp);
      EXPECT_EQ(TNY_OBJ, comp->type); // There is no float type. Swap is the same for INT32.
      EXPECT_EQ("m4", std::string(comp->key));
      cereal::CerealSerializeType<glm::mat4>::in(dictRoot, "m4", data);  // Will lookup the value in dictRoot.

      CompMat4 compareComp(data);
      compareComp.checkEqual(mat4Components[compID]);
    }

    return obj;
  };

  auto verifyDocumentStructure = [rootID, &checkCompM3, &checkCompM4](Tny* doc)
  {
    uint64_t compID;

    // Now check that the document has been serialized in the correct order
    // and has the correct components *and* values.
    //int32_t idata;
    ASSERT_EQ(TNY_DICT, doc->type);

    ASSERT_EQ(1, Tny_hasNext(doc));
    doc = Tny_next(doc);
    ASSERT_EQ("ren:mat3", std::string(doc->key));
    ASSERT_EQ(TNY_OBJ, doc->type);
    {
      Tny* obj = doc->value.tny;
      ASSERT_EQ(TNY_ARRAY, obj->type);
      ASSERT_EQ(1, Tny_hasNext(obj));

      obj = Tny_next(obj);
      ASSERT_EQ(TNY_OBJ, obj->type);

      {
        Tny* typeHeader = obj->value.tny;
        ASSERT_EQ(TNY_DICT, typeHeader->type);
        ASSERT_EQ(1, typeHeader->size);   // could be 4!

        ASSERT_EQ(1, Tny_hasNext(typeHeader));
        typeHeader = Tny_next(typeHeader);
        ASSERT_EQ(std::string("m3"), typeHeader->key);
        ASSERT_EQ(TNY_BIN, typeHeader->type);
        ASSERT_EQ(std::string("glm::mat3"), static_cast<const char*>(typeHeader->value.ptr));
      }

      // Extract component object and iterate.
      obj = Tny_next(obj);
      ASSERT_EQ(TNY_OBJ, obj->type);

      {
        Tny* comp = obj->value.tny;

        ASSERT_EQ(TNY_ARRAY, comp->type);
        ASSERT_EQ(1, Tny_hasNext(comp));

        // Manually iterate through IDs.
        compID = rootID;
        comp = Tny_next(comp);
        comp = checkCompM3(compID, comp);

        compID += 1;
        comp = Tny_next(comp);
        comp = checkCompM3(compID, comp);

        compID += 1;
        comp = Tny_next(comp);
        comp = checkCompM3(compID, comp);
      }
    }

    ASSERT_EQ(1, Tny_hasNext(doc));
    doc = Tny_next(doc);
    ASSERT_EQ("ren:mat4", std::string(doc->key));
    ASSERT_EQ(TNY_OBJ, doc->type);
    {
      // Test comphompos component.
      Tny* obj = doc->value.tny;
      ASSERT_EQ(TNY_ARRAY, obj->type);
      ASSERT_EQ(1, Tny_hasNext(obj));

      // Check for the type header
      obj = Tny_next(obj);
      ASSERT_EQ(TNY_OBJ, obj->type);

      {
        Tny* typeHeader = obj->value.tny;
        ASSERT_EQ(TNY_DICT, typeHeader->type);
        ASSERT_EQ(1, typeHeader->size);   // could be 4!

        ASSERT_EQ(1, Tny_hasNext(typeHeader));
        typeHeader = Tny_next(typeHeader);
        ASSERT_EQ(std::string("m4"), typeHeader->key);
        ASSERT_EQ(TNY_BIN, typeHeader->type);
        ASSERT_EQ(std::string("glm::mat4"), static_cast<const char*>(typeHeader->value.ptr));
      }

      // Extract component object and iterate.
      obj = Tny_next(obj);
      ASSERT_EQ(TNY_OBJ, obj->type);

      {
        Tny* comp = obj->value.tny;

        ASSERT_EQ(TNY_ARRAY, comp->type);
        ASSERT_EQ(1, Tny_hasNext(comp));

        // Manually iterate through IDs.
        compID = rootID;
        comp = Tny_next(comp);
        comp = checkCompM4(compID, comp);

        compID += 2;      // We skipped a mat4 component.
        comp = Tny_next(comp);
        comp = checkCompM4(compID, comp);
      }
    }
  };

  verifyDocumentStructure(root);

  // Now that we have verified the document, try and load the document back in
  // and re-walk the system to see if we get the same valid components.
  // This is really the only check we want to do, but the above is a sanity
  // check regarding the structure of the generated document.

  // Clear all components.
  core->clearAllComponentContainers();

  // Now serialize all components again.
  core->deserializeComponentCreate(root);

  // Check to make sure all of the components made it back in.
  core->renormalize(true);
  sysBasic->walkComponents(*core);
  sysOne->walkComponents(*core);

  // Appropriately kill the root.
  Tny_free(root);

  // Verify the document structure one more time.
  root = core->serializeAllComponents();
  verifyDocumentStructure(root);

  Tny_free(root);

  // Test serializing to a data pointer, and reserializing the data.
  root = core->serializeAllComponents();
  auto tnyDump = cereal::CerealCore::dumpTny(root);
  void* data = std::get<0>(tnyDump);
  size_t dataSize = std::get<1>(tnyDump);
  Tny_free(root);

  // Clear all of the components before reserializing.
  core->clearAllComponentContainers();

  Tny* reserialized = cereal::CerealCore::loadTny(data, dataSize);
  cereal::CerealCore::freeTnyDataPtr(data);

  core->deserializeComponentCreate(reserialized);
  Tny_free(reserialized);

  // Go through one more verification step.
  core->renormalize(true);
  sysBasic->walkComponents(*core);
  sysOne->walkComponents(*core);

  // Verify the document structure one more time.
  root = core->serializeAllComponents();
  verifyDocumentStructure(root);
}

}

