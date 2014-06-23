
#include <entity-system/GenericSystem.hpp>
#include <entity-system/ESCore.hpp>
#include <es-cereal/CerealCore.hpp>
#include <cereal-glm/CerealGLM.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <glm/glm.hpp>

namespace es = CPM_ES_NS;
namespace cereal = CPM_ES_CEREAL_NS;

namespace {

// We may want to enforce that these components have bson serialization members
// (possibly a static assert?).

struct CompPosition
{
  CompPosition() {}
  CompPosition(const glm::vec3& pos) {position = pos;}

  void checkEqual(const CompPosition& pos) const
  {
    EXPECT_FLOAT_EQ(position.x, pos.position.x);
    EXPECT_FLOAT_EQ(position.y, pos.position.y);
    EXPECT_FLOAT_EQ(position.z, pos.position.z);
  }

  // What this 'struct' is all about -- the data.
  glm::vec3 position;

  static const char* getName() {return "render:CompPosition";}

  static int SerializeCalls;
  bool serialize(cereal::ComponentSerialize& s, uint64_t /* entityID */)
  {
    ++SerializeCalls;
    s.serialize("pos", position);
    return true;
  }
};

int CompPosition::SerializeCalls = 0;

struct CompHomPos
{
  CompHomPos() {}
  CompHomPos(const glm::vec4& pos) {position = pos;}

  void checkEqual(const CompHomPos& pos) const
  {
    EXPECT_FLOAT_EQ(position.x, pos.position.x);
    EXPECT_FLOAT_EQ(position.y, pos.position.y);
    EXPECT_FLOAT_EQ(position.z, pos.position.z);
    EXPECT_FLOAT_EQ(position.w, pos.position.w);
  }

  // DATA
  glm::vec4 position;

  static const char* getName() {return "render:CompHomPos";}

  static int SerializeCalls;
  bool serialize(cereal::ComponentSerialize& s, uint64_t /* entityID */)
  {
    ++SerializeCalls;
    s.serialize("pos", position);
    return true;
  }
};
int CompHomPos::SerializeCalls = 0;

struct CompGameplay
{
  CompGameplay() : health(0), armor(0) {}
  CompGameplay(int healthIn, int armorIn)
  {
    this->health = healthIn;
    this->armor = armorIn;
  }

  void checkEqual(const CompGameplay& gp) const
  {
    EXPECT_EQ(health, gp.health);
    EXPECT_EQ(armor, gp.armor);
  }

  // DATA
  int32_t health;
  int32_t armor;

  static const char* getName() {return "render:CompGameplay";}

  static int SerializeCalls;
  bool serialize(cereal::ComponentSerialize& s, uint64_t /* entityID */)
  {
    ++SerializeCalls;
    s.serialize("health", health);
    s.serialize("armor", armor);
    return true;
  }
};
int CompGameplay::SerializeCalls = 0;

// Component positions. associated with id. The first component is not used.
std::vector<CompPosition> posComponents = {
  glm::vec3(0.0, 0.0, 0.0),
  glm::vec3(1.0, 2.0, 3.0),
  glm::vec3(5.5, 6.0, 10.7),
  glm::vec3(1.5, 3.0, 107),
  glm::vec3(4.0, 7.0, 9.0),
  glm::vec3(2.92, 89.0, 4.0),
};

std::vector<CompHomPos> homPosComponents = {
  glm::vec4(0.0, 0.0, 0.0, 0.0),
  glm::vec4(1.0, 11.0, 41.0, 51.0),
  glm::vec4(2.0, 12.0, 42.0, 52.0),
  glm::vec4(3.0, 13.0, 43.0, 53.0),
  glm::vec4(4.0, 14.0, 44.0, 54.0),
  glm::vec4(5.0, 15.0, 45.0, 55.0),
};

std::vector<CompGameplay> gameplayComponents = {
  CompGameplay(0, 0),
  CompGameplay(45, 21),
  CompGameplay(23, 123),
  CompGameplay(99, 892),
  CompGameplay(73, 64),
  CompGameplay(23, 92),
};

// This basic system will apply, every frame, to entities with the CompPosition,
// CompHomPos, and CompGameplay components.
class BasicSystem : public es::GenericSystem<false, CompPosition, CompHomPos, CompGameplay>
{
public:

  static std::map<uint64_t, bool> invalidComponents;

  void execute(es::ESCoreBase&, uint64_t entityID,
               const CompPosition* pos, const CompHomPos* homPos,
               const CompGameplay* gp) override
  {
    // Check to see if this entityID should have been executed.
    if (invalidComponents.find(entityID) != invalidComponents.end())
      FAIL() << "BasicSystem attempt to execute on an invalid component." << std::endl;

    // Check the values contained in each of pos, homPos, and gp.
    pos->checkEqual(posComponents[entityID]);
    homPos->checkEqual(homPosComponents[entityID]);
    gp->checkEqual(gameplayComponents[entityID]);
  }

  // Compile time polymorphic function required by CerealCore when registering.
  static const char* getName()
  {
    return "render:BasicSystem";
  }
};

class SystemOne : public es::GenericSystem<false, CompHomPos, CompGameplay>
{
public:
  static std::map<uint64_t, bool> invalidComponents;

  void execute(es::ESCoreBase&, uint64_t entityID,
               const CompHomPos* homPos,
               const CompGameplay* gp) override
  {
    // Check to see if this entityID should have been executed.
    if (invalidComponents.find(entityID) != invalidComponents.end())
      FAIL() << "SystemOne attempt to execute on an invalid component." << std::endl;

    // Check the values contained in each of pos, homPos, and gp.
    homPos->checkEqual(homPosComponents[entityID]);
    gp->checkEqual(gameplayComponents[entityID]);
  }
  
  // Compile time polymorphic function required by CerealCore when registering.
  static const char* getName()
  {
    return "render:SystemOne";
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
  core->registerComponent<CompPosition>();
  core->registerComponent<CompHomPos>();
  core->registerComponent<CompGameplay>();

  // Destroy the core and re-register components.
  // Then deserialize the data from memory to see if the correct components
  // and systems are serialized back in.

  uint64_t rootID = core->getNewEntityID();
  uint64_t id = rootID;
  core->addComponent(id, posComponents[id]);
  core->addComponent(id, homPosComponents[id]);
  core->addComponent(id, gameplayComponents[id]);

  id = core->getNewEntityID();
  core->addComponent(id, homPosComponents[id]);
  core->addComponent(id, gameplayComponents[id]);
  BasicSystem::invalidComponents.insert(std::make_pair(id, true));

  id = core->getNewEntityID();
  core->addComponent(id, posComponents[id]);
  core->addComponent(id, homPosComponents[id]);
  core->addComponent(id, gameplayComponents[id]);

  id = core->getNewEntityID();
  core->addComponent(id, posComponents[id]);
  core->addComponent(id, homPosComponents[id]);
  core->addComponent(id, gameplayComponents[id]);
  
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
  ASSERT_TRUE(CPM_ES_NS::TemplateID<CompPosition>::getID() < CPM_ES_NS::TemplateID<CompHomPos>::getID());
  ASSERT_TRUE(CPM_ES_NS::TemplateID<CompHomPos>::getID() < CPM_ES_NS::TemplateID<CompGameplay>::getID());

  auto checkCompPosition = [](uint64_t compID, Tny* obj) -> Tny*
  {
    EXPECT_EQ(TNY_INT64, obj->type);
    EXPECT_EQ(compID, obj->value.num);
    EXPECT_EQ(1, Tny_hasNext(obj));
    obj = Tny_next(obj);
    EXPECT_EQ(TNY_OBJ, obj->type);

    glm::vec3 data;
    {
      Tny* dictRoot = obj->value.tny;
      Tny* comp = dictRoot;
      EXPECT_EQ(TNY_DICT, comp->type);
      EXPECT_EQ(1, Tny_hasNext(comp));

      // X Component
      comp = Tny_next(comp);
      EXPECT_EQ(TNY_OBJ, comp->type); // There is no float type. Swap is the same for INT32.
      EXPECT_EQ("pos", std::string(comp->key));
      cereal::CerealSerializeType<glm::vec3>::in(dictRoot, "pos", data);  // Will lookup the value in dictRoot.
      EXPECT_EQ(posComponents[compID].position.x, data.x);
      EXPECT_EQ(posComponents[compID].position.y, data.y);
      EXPECT_EQ(posComponents[compID].position.z, data.z);
    }

    return obj;
  };

  auto checkCompHomPos = [](uint64_t compID, Tny* obj) -> Tny*
  {
    EXPECT_EQ(TNY_INT64, obj->type);
    EXPECT_EQ(compID, obj->value.num);
    EXPECT_EQ(1, Tny_hasNext(obj));
    obj = Tny_next(obj);
    EXPECT_EQ(TNY_OBJ, obj->type);

    glm::vec4 data;
    {
      Tny* dictRoot = obj->value.tny;
      Tny* comp = dictRoot;
      EXPECT_EQ(TNY_DICT, comp->type);
      EXPECT_EQ(1, Tny_hasNext(comp));

      // X Component
      comp = Tny_next(comp);
      EXPECT_EQ(TNY_OBJ, comp->type); // There is no float type. Swap is the same for INT32.
      EXPECT_EQ("pos", std::string(comp->key));
      cereal::CerealSerializeType<glm::vec4>::in(dictRoot, "pos", data);  // Will lookup the value in dictRoot.
      EXPECT_EQ(homPosComponents[compID].position.x, data.x);
      EXPECT_EQ(homPosComponents[compID].position.y, data.y);
      EXPECT_EQ(homPosComponents[compID].position.z, data.z);
      EXPECT_EQ(homPosComponents[compID].position.w, data.w);
    }

    return obj;
  };

  auto checkCompGameplay = [](uint64_t compID, Tny* obj) -> Tny*
  {
    EXPECT_EQ(TNY_INT64, obj->type);
    EXPECT_EQ(compID, obj->value.num);
    EXPECT_EQ(1, Tny_hasNext(obj));
    obj = Tny_next(obj);
    EXPECT_EQ(TNY_OBJ, obj->type);

    int32_t idata;
    {
      Tny* dictRoot = obj->value.tny;
      Tny* comp = dictRoot;
      EXPECT_EQ(TNY_DICT, comp->type);
      EXPECT_EQ(1, Tny_hasNext(comp));

      // X Component
      comp = Tny_next(comp);
      EXPECT_EQ(TNY_INT32, comp->type); // There is no float type. Swap is the same for INT32.
      EXPECT_EQ("health", std::string(comp->key));
      cereal::CerealSerializeType<int32_t>::in(dictRoot, "health", idata);  // Will lookup the value in dictRoot.
      EXPECT_EQ(gameplayComponents[compID].health, idata);

      // Y Component
      comp = Tny_next(comp);
      EXPECT_EQ(TNY_INT32, comp->type); // There is no float type. Swap is the same for INT32.
      EXPECT_EQ("armor", std::string(comp->key));
      cereal::CerealSerializeType<int32_t>::in(dictRoot, "armor", idata);  // Will lookup the value in dictRoot.
      EXPECT_EQ(gameplayComponents[compID].armor, idata);
    }

    return obj;
  };

  auto verifyDocumentStructure = [rootID, &checkCompPosition, &checkCompHomPos, &checkCompGameplay](Tny* doc)
  {
    uint64_t compID;

    // Now check that the document has been serialized in the correct order
    // and has the correct components *and* values.
    //int32_t idata;
    ASSERT_EQ(TNY_DICT, doc->type);

    ASSERT_EQ(1, Tny_hasNext(doc));
    doc = Tny_next(doc);
    ASSERT_EQ("render:CompPosition", std::string(doc->key));
    ASSERT_EQ(TNY_OBJ, doc->type);
    {
      // Test position component.
      Tny* obj = doc->value.tny;
      ASSERT_EQ(TNY_ARRAY, obj->type);
      ASSERT_EQ(1, Tny_hasNext(obj));

      // Check for the type header
      obj = Tny_next(obj);
      ASSERT_EQ(TNY_OBJ, obj->type);

      // Check position type header.
      {
        Tny* typeHeader = obj->value.tny;
        ASSERT_EQ(TNY_DICT, typeHeader->type);
        ASSERT_EQ(1, typeHeader->size);   // could be 4!

        ASSERT_EQ(1, Tny_hasNext(typeHeader));
        typeHeader = Tny_next(typeHeader);
        ASSERT_EQ(std::string("pos"), typeHeader->key);
        ASSERT_EQ(TNY_BIN, typeHeader->type);
        ASSERT_EQ(std::string("glm::vec3"), static_cast<const char*>(typeHeader->value.ptr));
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
        comp = checkCompPosition(compID, comp);

        compID += 2;  // We skipped one CompPosition when adding components.
        comp = Tny_next(comp);
        comp = checkCompPosition(compID, comp);

        compID += 1;
        comp = Tny_next(comp);
        comp = checkCompPosition(compID, comp);
      }
    }

    ASSERT_EQ(1, Tny_hasNext(doc));
    doc = Tny_next(doc);
    ASSERT_EQ("render:CompHomPos", std::string(doc->key));
    ASSERT_EQ(TNY_OBJ, doc->type);
    {
      // Test comphompos component.
      Tny* obj = doc->value.tny;
      ASSERT_EQ(TNY_ARRAY, obj->type);
      ASSERT_EQ(1, Tny_hasNext(obj));

      // Check for the type header
      obj = Tny_next(obj);
      ASSERT_EQ(TNY_OBJ, obj->type);

      // Check hompos type header.
      {
        Tny* typeHeader = obj->value.tny;
        ASSERT_EQ(TNY_DICT, typeHeader->type);
        ASSERT_EQ(1, typeHeader->size);   // could be 4!

        ASSERT_EQ(1, Tny_hasNext(typeHeader));
        typeHeader = Tny_next(typeHeader);
        ASSERT_EQ(std::string("pos"), typeHeader->key);
        ASSERT_EQ(TNY_BIN, typeHeader->type);
        ASSERT_EQ(std::string("glm::vec4"), static_cast<const char*>(typeHeader->value.ptr));
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
        comp = checkCompHomPos(compID, comp);

        compID += 1;
        comp = Tny_next(comp);
        comp = checkCompHomPos(compID, comp);

        compID += 1;
        comp = Tny_next(comp);
        comp = checkCompHomPos(compID, comp);

        compID += 1;
        comp = Tny_next(comp);
        comp = checkCompHomPos(compID, comp);
      }
    }

    ASSERT_EQ(1, Tny_hasNext(doc));
    doc = Tny_next(doc);
    ASSERT_EQ("render:CompGameplay", std::string(doc->key));
    ASSERT_EQ(TNY_OBJ, doc->type);
    {
      // Test comphompos component.
      Tny* obj = doc->value.tny;
      ASSERT_EQ(TNY_ARRAY, obj->type);
      ASSERT_EQ(1, Tny_hasNext(obj));

      // Check for the type header
      obj = Tny_next(obj);
      ASSERT_EQ(TNY_OBJ, obj->type);

      // Check gameplay type header.
      {
        Tny* typeHeader = obj->value.tny;
        ASSERT_EQ(TNY_DICT, typeHeader->type);
        ASSERT_EQ(2, typeHeader->size);   // could be 4!

        ASSERT_EQ(1, Tny_hasNext(typeHeader));
        typeHeader = Tny_next(typeHeader);
        ASSERT_EQ(std::string("health"), typeHeader->key);
        ASSERT_EQ(TNY_BIN, typeHeader->type);
        ASSERT_EQ(std::string("int32"), static_cast<const char*>(typeHeader->value.ptr));

        ASSERT_EQ(1, Tny_hasNext(typeHeader));
        typeHeader = Tny_next(typeHeader);
        ASSERT_EQ(std::string("armor"), typeHeader->key);
        ASSERT_EQ(TNY_BIN, typeHeader->type);
        ASSERT_EQ(std::string("int32"), static_cast<const char*>(typeHeader->value.ptr));
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
        comp = checkCompGameplay(compID, comp);

        compID += 1;
        comp = Tny_next(comp);
        comp = checkCompGameplay(compID, comp);

        compID += 1;
        comp = Tny_next(comp);
        comp = checkCompGameplay(compID, comp);

        compID += 1;
        comp = Tny_next(comp);
        comp = checkCompGameplay(compID, comp);
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

