OR, XOR, AND = 1, 3, 4
function bitwise(a, b, operator)
  local r, m, s = 0, 2 ^ 31
  repeat
    s, a, b = a + b + m, a % m, b % m
    r, m = r + m * operator % (s - a - b), m / 2
  until m < 1
  return r
end

local atp_protocol = Proto("hexatp", "APDU/ATR tunneling protocol")
local version = ProtoField.uint32("hexatp.version", "Version", base.DEC, nil, 0xF0000000)
local type = ProtoField.uint32("hexatp.type", "Type", base.HEX, {
  [0] = "APDU",
  [1] = "ATR",
  [2] = "Unset card",
  [3] = "Stop"
}, 0x0F000000)
local reserved = ProtoField.uint32("hexatp.reserved", "Reserved", base.HEX, nil, 0x00FE0000)
local length = ProtoField.uint32("hexatp.length", "Length", base.DEC, nil, 0x0001FFFF)
local data = ProtoField.bytes("hexatp.data", "Data", base.SPACE)

atp_protocol.fields = {
  version,
  type,
  reserved,
  length,
  data,
}

function atp_protocol.dissector(buffer, pinfo, tree)
  local size = buffer:len()
  if size < 4 then return end

  pinfo.cols.protocol = "APDU/ATR Tunneling Protocol"

  local subtree = tree:add(atp_protocol, buffer(), "ATP Data")
  subtree:add_le(version, buffer(0, 4))
  subtree:add_le(type, buffer(0, 4))
  subtree:add_le(reserved, buffer(0, 4))
  subtree:add_le(length, buffer(0, 4))
  subtree:add_le(data, buffer(4, size - 4))
end

local tcp_port = DissectorTable.get("tcp.port")
tcp_port:add(9000, atp_protocol)
