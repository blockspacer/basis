## About

`multiconfig` - configuration system that can support multiple configuration providers.

Configuration providers can read value from some data source (json file, environment variables, etc.)

Imagine that you already have `json configuration loader` and `ini configuration loader`.

Some configuration options use `json configuration loader`: `my_json_guid`.

And configuration options some use `ini configuration loader`: `my_ini_id`.

You can use `multiconfig` to add configuration option `my_name` that can load from either `json configuration loader` or `ini configuration loader`.

```cpp
// "my_name" can load from ini or json files
MULTICONF_string(my_name, "my_default_value", {INI_MULTICONF_LOADER, JSON_MULTICONF_LOADER});

// "my_json_guid" can load only from json files
MULTICONF_string(my_json_guid, "my_default_value", {JSON_MULTICONF_LOADER});

// "my_ini_id" can load only from ini files
MULTICONF_string(my_ini_id, "my_default_value", {INI_MULTICONF_LOADER});
```

## Configuration groups

Imagine that you have multiple "plugins" and you need to avoid collision.

You can not create same option `my_key` using
`MULTICONF_String(my_key, "", MY_LOADERS);`
multiple times (even in different files or plugins).

Use multiple configuration groups to avoid collision like so:
#define CONFIG_GROUP_A "my_plugin_a"
#define CONFIG_GROUP_B "my_plugin_b"
`MULTICONF_String(my_key, "", MY_LOADERS, CONFIG_GROUP_A);`
`MULTICONF_String(my_key, "", MY_LOADERS, CONFIG_GROUP_B);`

It will create `my_plugin_a_my_key` and `my_plugin_b_my_key` configuration values (combines group name and option name).

For now we used configuration groups only to avoid collision.

They can also be used to reload on demand limited set of options
without need to refresh whole configuration.

That may help if you have configuration stored in multiple files,
each configuration group can be associated with individual file.

See:

```cpp
basis::StatusOr<std::string> tryLoadString(
  const std::string& name, const std::string& configuration_group)
```

The basic idea is that your configuration loader can load file based on provided `configuration_group`.

```cpp
// load file with name based on `configuration_group`!
std::string json_loaded = loadFileByPath(configuration_group);
// no need to combine `name` and `configuration_group` here!
std::string result = getValueByJsonKey(name);
```

Make sure that `configuration_group` is used somehow to avoid collision.
If `configuration_group` does not affect anything, than at least use it to create combined key name:

```cpp
// load configuration option with title `key`, but not with title `name`!
std::string key = formatConfigNameAndGroup(name, configuration_group);
// used combined `name` and `configuration_group` here!
std::string result = getEnvironmentVariableByName(key);
```

## Usage

Syntax used to add new configuration option:

```cpp
MULTICONF_typename(NAME, DEFAULT_VALUE, INITIALIZER_LIST_OF_LOADERS);
```

Syntax used to add multiple configuration options:

```cpp
// creates var. `my_string_key` and configuration option using text "my_string_key"
MULTICONF_String(my_string_key, "-12345", BUILTIN_MULTICONF_LOADERS);
// creates var. `my_bool_key` and configuration option using text "my_bool_key"
MULTICONF_Bool(my_bool_key, "True", BUILTIN_MULTICONF_LOADERS);
MULTICONF_Int(my_int_key, "-12345", BUILTIN_MULTICONF_LOADERS);
```

Note that `MULTICONF_Bool(my_bool_key, ...` will declare variable `basis::MultiConfValueObserver<bool> my_bool_key = ...`.

So make sure that you place `MULTICONF_` inside anonymous namespace or struct/class/etc.

It is also valid to use `static` like so: `static MULTICONF_Bool(my_bool_key, "True", BUILTIN_MULTICONF_LOADERS);`

To use registered configuration options:

```cpp
// required before first usage
CHECK_OK(basis::MultiConf::GetInstance().init());

// you can reload confuguration on demand
CHECK_OK(basis::MultiConf::GetInstance().clearAndReload());

// may crash if `string` not convertable to `bool`, see `error_status().ok()`
std::string str = my_string_key.GetValue();

// may crash if `string` not convertable to `bool`, see `error_status().ok()`
bool flag = my_bool_key.GetValue();
```

Note that without `clearAndReload` configuration values will be cached
i.e. changes in configuration files are NOT auto detected (but you can add that functionality manually, see `addObserver`).

Note that `GetValue` may fail if provided configuration is broken
(for example, if `my_int_key` can not be converted to `int`  from `string`),
so you can check for errors using `error_status().ok()`.

## How to add custom configuration provider

Imagine that you want to load some configuration option from either json file environment variable.

Lets create configuration provider that can read environment variables.

Each configuration provider must have public `id` and function that returns `basis::StatusOr<std::string>` based on provided `string`:

```cpp
basis::StatusOr<std::string> tryLoadString(
  const std::string& name, const std::string& configuration_group);
static constexpr char kId[] = "EnvMultiConf";
```

Example:

```cpp
// Configuration loader that uses environment vars
class EnvMultiConf {
 public:
  // Thread safe GetInstance.
  static EnvMultiConf& GetInstance();

  // Loads configuraton value from environment vars
  basis::StatusOr<std::string> tryLoadString(
    const std::string& name, const std::string& configuration_group);

 public:
  // id for debug purposes
  static constexpr char kId[] = "EnvMultiConf";

 private:
  EnvMultiConf();

 private:
  std::unique_ptr<base::Environment> env_;

  friend class base::NoDestructor<EnvMultiConf>;

  DISALLOW_COPY_AND_ASSIGN(EnvMultiConf);
};


EnvMultiConf::EnvMultiConf()
  : env_{base::Environment::Create()}
{}

EnvMultiConf& EnvMultiConf::GetInstance() {
  // C++11 static local variable initialization is
  // thread-safe.
  static base::NoDestructor<EnvMultiConf> instance;
  return *instance;
}

basis::StatusOr<std::string> EnvMultiConf::tryLoadString(
  const std::string& name, const std::string& configuration_group)
{
  std::string key = FORMAT_NAME_AND_GROUP(name, configuration_group);

  DCHECK(!key.empty());

  std::string result;

  if(env_->GetVar(key, &result))
  {
    return result;
  }

  RETURN_ERROR()
    << " ("
    << kId
    << ") unable to find key in environment variables: "
    << key;
}
```

Now you can pass created configuration provided when you create new configuration options using `MULTICONF_`

Example:

```cpp
#define ENV_MULTICONF_LOADER \
    basis::MultiConfLoader{ \
      ::basis::EnvMultiConf::kId \
      , base::BindRepeating( \
          &::basis::EnvMultiConf::tryLoadString \
          , base::Unretained(&EnvMultiConf::GetInstance())) \
    }

// note that each configuration option can have custom set of configuration loaders
MULTICONF_String(my_conf_key, "abcd", {ENV_MULTICONF_LOADER, JSON_MULTICONF_LOADER});
```

## Implementation details

Configuration provider can return only string value (see `tryLoadString`)
i.e. even if you read `int` from json file, it will be stored as `string` internally.

`GetValue()` returs cached value.

Cached value may be any data type that was parsed from `string`.

`GetValue()` does not parse `string` on each use.

We use observer pattern to detect configuration changes and invalidate only required values in cache, see `addObserver`.

Each `MULTICONF_*` macro expected to create single wariable,
so you will be able to write code: `static MULTICONF_String(...)`.
