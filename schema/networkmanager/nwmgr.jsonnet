// The schema used by NetworkManager

local moo = import "moo.jsonnet";

// A schema builder in the given path (namespace)
local s = moo.oschema.schema("dunedaq.networkmanager.nwmgr");

// Object structure used by NetworkManager
local nm = {
  name: s.string("Name", doc="Logical name of the connection"),
  
  address: s.string("Address", doc="Address of endpoint"),

  topic: s.string("Topic", doc="A topic on a connection"),
  topics: s.sequence("Topics", self.topic, doc="List of topics on a connection"),

  conninfo: s.record("Connection", [
  s.field("name", self.name, "", doc="Logical name of the connection"),
  s.field("address", self.address, "", doc="Address of endpoint"),
  s.field("topics", self.topics, doc="Topics on this connection")
  ], doc="Information about a connection"),

  connections: s.sequence("Connections", self.conninfo, doc="List of connection information objects"),
  
//  conf: s.record("Conf",  [
//    s.field("connections", self.connections, [],
//      doc="List of connection information objects")
//   ], doc="NetworkManager Configuration"),
  
};

moo.oschema.sort_select(nm)

