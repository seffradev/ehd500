add_rules("mode.debug", "mode.release")

add_repositories("seffradev git@github.com:seffradev/xmake-repo")
add_requires("libpcsc")
-- add_requires("libosmocore", { system = false })
add_requires("gtest", {
  configs = {
    main = false,
    gmock = true,
  },
})

if is_mode("debug") then
  set_symbols("debug")
  set_optimize("none")
  -- set_policy("build.sanitizer.address", true)
  -- set_policy("build.sanitizer.undefined", true)
  -- set_policy("build.sanitizer.leak", true)
  -- set_policy("build.sanitizer.thread", true)
else
  set_symbols("hidden")
  set_optimize("fastest")
  set_strip("all")
end

set_warnings("all", "error")
set_languages("cxxlatest")

target("simbank")
set_kind("binary")
add_files("src/simbank.cc")
add_includedirs("include", { public = true })
add_packages("libpcsc")
add_links("pcsclite")
set_default(false)

target("consumer")
set_kind("binary")
add_files("src/consumer.cc")
add_includedirs("/usr/include/libusb-1.0/")
add_includedirs("include", { public = true })
add_syslinks("osmo-simtrace2", "osmosim", "osmousb", "osmocore", "usb-1.0")
-- add_packages("libosmocore")
set_default(true)

-- Iterate over test files and create test targets
for _, file in ipairs(os.files("tests/**.cc")) do
  local name = path.basename(file)
  target(name)
  set_kind("binary")
  add_packages("gtest")
  add_packages("libpcsc")
  add_links("pcsclite")
  add_links("gtest_main")
  add_includedirs("/usr/include/libusb-1.0/")
  add_includedirs("include", { public = true })
  add_syslinks("osmo-simtrace2", "osmosim", "osmousb", "osmocore", "usb-1.0")
  set_default(false)
  add_files(file)
  add_deps("simbank", "consumer")
  add_tests("default")
end
