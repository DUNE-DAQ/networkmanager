// This is the application info schema used by the network manager.
// It describes the information object structure passed by the application 
// for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.networkmanager.nwmgrreceivedinfo");

local info = {

   count  : s.number("count", "u8", doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("bytes", self.count, doc="Bytes received via a connection of the networkmanager")
       ], doc="Netowrk Manager information")
};

moo.oschema.sort_select(info) 
