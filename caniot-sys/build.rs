use std::{env, path::PathBuf};

const HEADERS: &[&str] = &[
    "../include/caniot/caniot_config.h",
    "../include/caniot/caniot.h",
    "../include/caniot/caniot_private.h",
    "../include/caniot/classes.h",
    "../include/caniot/controller.h",
    "../include/caniot/datatype.h",
    "../include/caniot/device.h",
    "../include/caniot/drivers",
    "../include/caniot/errors.h",
    "../include/caniot/fake.h",
    "../include/caniot/phys.h",
    "../include/caniot/drivers/linux.h",
    "../include/caniot/drivers/dummy.h",
];

const HEADERS_WORLD: &[&str] = &[
    "../include/caniot/caniot.h",
    "../include/caniot/controller.h",
    "../include/caniot/datatype.h",
    "../include/caniot/device.h",
    "../include/caniot/classes.h",
    "../include/caniot/drivers/linux.h",
    "../include/caniot/drivers/dummy.h",
];

const CONF: &[&str] = &[
    "-DCONFIG_CANIOT_CHECKS=1",
    "-DCONFIG_CANIOT_DRIVERS_API=1",
    "-DCONFIG_CANIOT_DEVICE_HANDLE_BLC_SYS_CMD=1",
    "-DCONFIG_CANIOT_CTRL_DRIVERS_API=1",
    "-DCONFIG_CANIOT_DEBUG=1",
    "-DCONFIG_CANIOT_DEVICE_FILTER_FRAME=1",
    "-DCONFIG_CANIOT_DEVICE_STARTUP_ATTRIBUTES=1",
    "-DCONFIG_CANIOT_MAX_PENDING_QUERIES=4", // 32 for production
    "-DCONFIG_CANIOT_ATTRIBUTE_NAME=1",
    "-DCONFIG_CANIOT_CONTROLLER_DISCOVERY=1",
    "-DCONFIG_CANIOT_ASSERT=1",
    "-DCONFIG_CANIOT_QUERY_ID=1",
    "-DCONFIG_CANIOT_BUILD_INFOS=1",
    "-DCONFIG_CANIOT_LOG_LEVEL=4", // 4 for debug
    "-DCONFIG_CANIOT_POSIX=1",
    "-DCONFIG_CANIOT_DRIVER_LINUX=1",
];

const OPAQUE_TYPES: &[&str] = &[
    "caniot_blc0_telemetry",
    "caniot_blc0_command",
    "caniot_blc1_telemetry",
    "caniot_blc1_command",
];

fn bindgen() {
    let mut bindings = bindgen::Builder::default()
        .layout_tests(false)
        .use_core()
        .formatter(bindgen::Formatter::Rustfmt)
        .default_enum_style(bindgen::EnumVariation::ModuleConsts)
        .size_t_is_usize(false)
        .clang_arg("-I../src")
        .clang_arg("-I../include")
        .allowlist_item("CANIOT.*")
        .allowlist_item("linux_driver_api_ptr")
        .allowlist_item("dummy_driver_api_ptr")
        .derive_eq(true)
        .derive_partialeq(true)
        .layout_tests(true);

    for opaque_type in OPAQUE_TYPES {
        bindings = bindings.opaque_type(*opaque_type);
    }

    let bindings = HEADERS_WORLD
        .iter()
        .fold(bindings, |b, header| b.header(*header));

    let bindings = CONF.iter().fold(bindings, |b, conf| b.clang_arg(*conf));

    let allowlist_functions = ["caniot_.*"];
    let allowlist_types = ["caniot_.*"];

    let bindings = allowlist_functions
        .iter()
        .fold(bindings, |b, func| b.allowlist_function(func));

    let bindings = allowlist_types
        .iter()
        .fold(bindings, |b, ty| b.allowlist_type(ty));

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}

fn main() {
    let caniot_lib_path = env::var("CANIOT_LIB_PATH").unwrap_or_else(|_| "out".to_string());
    let caniot_lib_type = env::var("CANIOT_LIB_TYPE").unwrap_or_else(|_| "static".to_string());

    // link the C library
    // println!("cargo:rustc-link-lib=caniot");
    println!("cargo:rustc-link-search={}", caniot_lib_path);

    match caniot_lib_type.as_str() {
        "static" => println!("cargo:rustc-link-lib=static=caniot"),
        _ => println!("cargo:rustc-link-lib=dylib=caniot"),
    }

    for header in HEADERS {
        println!("cargo:rerun-if-changed={}", header);
    }

    println!("cargo:rerun-if-changed={}/libcaniot.a", caniot_lib_path);
    println!("cargo:rerun-if-changed={}/libcaniot.so", caniot_lib_path);

    bindgen();
}
