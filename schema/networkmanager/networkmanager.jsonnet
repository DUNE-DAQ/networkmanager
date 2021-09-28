// The schema used by NetworkManager

local moo = import "moo.jsonnet";

// A schema builder in the given path (namespace)
local s = moo.oschema.schema("dunedaq.networkmanager.networkmanager");

// Object structure used by NetworkManager
local nm = {
  name: s.string("Name", doc="Logical name of the connection"),

  receivertype: s.enum("Type", ["Receiver", "Subscriber"], doc="Receiver plugin type"),
  
  address: s.string("Address", doc="Address of endpoint"),

  conninfo: s.record("Connection", [
  s.field("name", self.name, "", doc="Logical name of the connection"),
  s.field("type", self.receivertype, "Receiver", doc="Plugin type for receives"),
  s.field("address", self.address, "", doc="Address of endpoint"),
  ], doc="Information about a connection"),

  connections: s.sequence("Connections", self.conninfo, doc="List of connection information objects"),
  
  conf: s.record("Conf",  [
    s.field("connections", self.connections, [],
      doc="List of connection information objects")
   ], doc="NetworkManager Configuration"),
  
};

moo.oschema.sort_select(nm)

