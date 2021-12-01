// This is the application info schema used by the network manager.
// It describes the information object structure passed by the application 
// for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.networkmanager.networkmanagerinfo");

local info = {
   count  : s.number("count", "u8", doc="An unsigned of 8 bytes"),
   connection  : s.string("connection", doc="connection name"),

   entry : s.record( "entry", [
   	 s.field( "connection", self.connection, "", doc="connection name"), 
	 s.field( "bytes", self.count, 0, doc="bytes sent or received via the connection")
	 ], doc="single entry for a report" ),

   list : s.sequence( "list", self.entry, doc="list of entries in the metric report" ),

   info: s.record("Info", [
       s.field("received", self.list, doc="list of received bytes, divided by connection"),
       s.field("sent", self.list, doc="list of sent bytes, divided by connection")
       ], doc="Netowrk Manager information")
};

moo.oschema.sort_select(info) 
