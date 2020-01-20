To make it possible upgrading plugin (without doing uninstallation at first) Product Id has to be changed in Product.wxs
along with the plugin version. Plugin version most likely will be updated automatically while building the plugin.
So only new guid for the Product will be needed when creating a new installer. GenGuid script provides this automatically.
