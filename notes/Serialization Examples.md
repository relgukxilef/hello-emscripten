Serialization Examples

```c++
struct profile {
    unsigned id;
    buffer<char> display_name;
    buffer<buffer<char>> aliases;
    buffer<char> avatar_url;
    bool bot;
};
```

```json
{
    "id": 1,
    "display_name": "Felix",
    "aliases": [
        "relgukxilef"
    ],
    "avatar_url": "https://hellovr.at/a/Felix/Miku.gltf",
    "bot": false
}
```

```SQL
REPLACE INTO profiles (id, display_name, avatar_url, bot) 
VALUES (1, "Felix", "https://hellovr.at/a/Felix/Miku.gltf", FALSE)

REPLACE INTO aliases (profile_id, display_name) 
VALUES (1, "relgukxilef")
```


```c++
apply(v, key("id") | PRIMARY_KEY, &id);
apply(v, key("display_name") | TEXT, &display_name);
    apply(v, SIZE, &display_name.size);
    apply(v, CHARACTER, &display_name.data[0]);
    ...
apply(v, key("aliases") | ARRAY, &aliases);
    apply(v, SIZE, &aliases.size);
    apply(v, TEXT, &aliases.data[0]);
        apply(v, SIZE, &aliases.data[0].size);
        apply(v, CHARACTER, &aliases.data[0].data[0]);
        ...
apply(v, key("avatar_url") | URL, &avatar_url);
    apply(v, SIZE, &avatar_url.size);
    apply(v, CHARACTER, &avatar_url.data[0]);
    ...
apply(v, key("bot") | BOOLEAN, &bot);
```