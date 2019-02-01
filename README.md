# News

On Feb 1, 2019, Gobbledegook's license changed from GPL to LGPL in hopes that it would be found useful to more developers.

# What is Gobbledegook?

_Gobbledegook_ is a C/C++ standalone Linux [Bluetooth LE](https://en.wikipedia.org/wiki/Bluetooth_Low_Energy) [GATT](https://www.bluetooth.com/specifications/gatt/generic-attributes-overview) server using [BlueZ](http://www.bluez.org/about/) over [D-Bus](https://www.freedesktop.org/wiki/Software/dbus/#index1h1) with [Bluetooth Management API](https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/mgmt-api.txt) support built in. That's a lot of words, so I shortened it to _Gobbledegook_. Then I shortened it again to _GGK_ because let's be honest, it's a pain to type.

For the impatient folks in a hurry (or really just have to pee) skip down to the **Quick-start for the impatient** section at the bottom of this document.

### Features

* DSL-like creation of BLE services makes creating services trivial
* Automated D-Bus object hierarchy introspection generation
* Automated D-Bus ObjectManager implementation
* Automated BlueZ GATT application registration
* Support for the [Bluetooth Management API](https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/mgmt-api.txt)
* Timer events allow services to perform periodic updates
* Application-managed server data
* Comment blocks at the top of each source file with deep explanations of critical concepts
* Written in C++14 (gcc & clang) with a standard C public interface
* Tested on Ubuntu 16.04 on x86 and Raspberry Pi

# Server description

A *server description* is the meat of your server. It is a collection one or more GATT Services. Each GATT Service contains a collection of one or more characteristics, which in turn may contain zero or more descriptors. The server description is declared in the `Server` constructor found in `Server.cpp`.

The following sections explain how to build a server description.

> **New to Bluetooth terminology?**
>
> **Bluetooth Low Energy** (or **BLE**) is the marketing term for Bluetooth 4.0. A *BLE* server will offer up one or more *GATT Services*.
>
> **GATT** stands for Generic Attribute Profile (apparently the *P* is silent.) This is the standard that defines how BLE devices share data. Specifically, it defines the structure of services, characteristics and descriptors.
>
>A **GATT Service** can be thought of as a discreet unit of functionality. Examples would be a *time service* that returns the current local time and timezone or a *battery service* that returns the current battery level and temperature. A *GATT Service* serves up information in the form of a collection of one or more *GATT Characteristics*.
>
>A **GATT Characteristic** is one unit of data from a GATT Service. For example, our fictitious *battery service* would have two *GATT Characteristics*: (1) the current battery level and (2) the temperature. The battery level characteristic might be defined as a single byte value in the range of 0-100 representing the percentage of battery remaining. In addition to their data, a *GATT Characteristic* may also contain zero or more optional *GATT Descriptors*.
> 
> A **GATT Descriptor** contains additional information and metadata about a *GATT Characteristic*. They can be used to describe the characteristic's features or to control certain behaviors of the characteristic. Extending our *battery service* example further, the temperature characteristic could be a 16-bit value representing a temperature, while the *GATT Descriptor* further defines how to interpret that data (as degrees Fahrenheit, degrees Celsius or 10th of degrees Kelvin.)

### Implementing services with GGK

Below is a complete custom GATT Service as defined within the GGK framework. It is a simple service that uses the `asctime()` function to return the current time as a string:

```c++
// Create a service
.gattServiceBegin("ascii_time", "00000001-1E3D-FAD4-74E2-97A033F1BFEE")

	// Add a characteristic to the service with the 'read' flag set
	.gattCharacteristicBegin("string", "00000002-1E3D-FAD4-74E2-97A033F1BFEE", {"read"})

		// Handle the characteristic's "ReadValue" method
		.onReadValue(CHARACTERISTIC_METHOD_CALLBACK_LAMBDA
		{
			time_t timeVal = time(nullptr);
			struct tm *pTimeStruct = localtime(&timeVal);
			std::string timeString = asctime(pTimeStruct);
			self.methodReturnValue(pInvocation, timeString, true);
		})

	.gattCharacteristicEnd()

.gattServiceEnd()
```

The first thing you may notice about this example is that many of the lines begin with a dot. This is because we're chaining methods together. Each method returns the appropriate type to provide *context*. Internally the `gattServiceBegin()` method returns a reference to a `GattService` object which provides the proper context to create a characteristic within that service. Similarly, the `gattCharacteristicBegin()` method returns a reference to a `GattCharacteristic` object which provides the proper context for responding to the `onReadValue()` method or adding descriptors to the characteristic.

You may also have noticed that we're using lambdas to include our implementation inline. The code to generate the time string is wrapped up in a `CHARACTERISTIC_METHOD_CALLBACK_LAMBDA` which is just a convenience macro that declares the lambda properly for us. You can use the raw lambda declaration if you wish, but then you're being anti-macroist and that's just not cool, bruh. And if you don't like these new-fangled lambdas, you can just stick a good ol' function pointer in its place.

>#### *Side note*
>
>A compiled GGK library provides a public interface that is compatible with standard C, but you'll need a modern compiler to build a GGK library because the internals are written using features of c++11.

Let's take a look at a more complex example. Here's an implementation of the Bluetooth standard's [Current Time Service](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.current_time.xml). We'll even toss a few extras in to keep things interesting:

```c++
// Current Time Service (0x1805)
.gattServiceBegin("time", "1805")
	.gattCharacteristicBegin("current", "2A2B", {"read", "notify"})
		.onReadValue(CHARACTERISTIC_METHOD_CALLBACK_LAMBDA
		{
			self.methodReturnVariant(pInvocation, ServerUtils::gvariantCurrentTime(), true);
		})
		.gattDescriptorBegin("description", "2901", {"read"})
			.onReadValue(DESCRIPTOR_METHOD_CALLBACK_LAMBDA
			{
				self.methodReturnValue(pInvocation, "Current server local time", true);
			})
		.gattDescriptorEnd()
		.onEvent(60, nullptr, CHARACTERISTIC_EVENT_CALLBACK_LAMBDA
		{
			self.sendChangeNotificationVariant(pConnection, ServerUtils::gvariantCurrentTime());
		})
	.gattCharacteristicEnd()
	.gattCharacteristicBegin("local", "2A0F", {"read"})
		.onReadValue(CHARACTERISTIC_METHOD_CALLBACK_LAMBDA
		{
			self.methodReturnVariant(pInvocation, ServerUtils::gvariantLocalTime(), true);
		})
		.gattDescriptorBegin("description", "2901", {"read"})
			.onReadValue(DESCRIPTOR_METHOD_CALLBACK_LAMBDA
			{
				self.methodReturnValue(pInvocation, "Local time data", true);
			})
		.gattDescriptorEnd()
	.gattCharacteristicEnd()
.gattServiceEnd()
```

If you're already familiar with BLE, then hopefully the expansion to multiple characteristics and the addition of descriptors needs no further explanation. If that's true, then you're probably amazed by that. Maybe a more modest level of amazement than it's-bigger-on-the-inside amazement levels, but you should still be sure to catch your breath before trying to read further. Safety first.

Did you notice the bonus call to `onEvent()`? The event (a `TickEvent` to be specific) is not part of the Bluetooth standard. It works similar to a typical GUI timer event. In this example, we're using it to send out a change notification (a "PropertiesChanged" notification in the standard parlance). Any client that has subscribed to that characteristic will receive an updated time every 60 ticks (seconds.)

### Contexts

Working in hierarchical contexts of *services*, *characteristics*, *descriptors* and *methods* simplifies the process building a server description because each context has a limited set of available tools to work with. For example, within a *service* context the only tools available are `gattCharacteristicBegin()` and `gattServiceEnd()`. This isn't a limitation on your flexibility; anything else would be a deviation from the [specification](https://git.kernel.org/pub/scm/bluetooth/bluez.git/plain/doc/gatt-api.txt).

>#### *TIP: A context is a scope*
>
>A GGK *context* is a synonym for the C term *scope*.
>
>The *Service context* is another way of saying the *`GattService` object scope*. Similarly, *Characteristics* and *Descriptors* are scopes of the `GattCharacteristic` and `GattDescriptor` objects.
>
>*Methods* differ slightly in that they are scoped to their lambdas. However, *Methods* also contain a `self` parameter which is a reference to the containing scope. In other words, a *Method* within a *Descriptor* will have a `self` reference to the `GattDescriptor` object where that method is declared. More on this in the **Lambda reference** section.

### Service cheat sheet

GATT Services are chained together to form a complete *server description*. So let's focus on how a GATT Service is built.

Below is a template for a GATT Service with all available components. Function parameters are shown by name and indentation denotes context.

Note that a service may contain more than one characteristic, in which case they would follow each other sequentially within the service description. The same holds true for Descriptors, except that a characteristic may have no descriptors. In addition, each of the four methods found in the characteristics and descriptors (`.onEvent()`, `.onReadValue()`, `.onWriteValue()` and `.onUpdateValue()`) would only be present if needed.

	.gattServiceBegin(name, uuid)
	    .gattCharacteristicBegin(name, uuid, flags[])
	        .onEvent(tickFrequency, userData, CHARACTERISTIC_EVENT_CALLBACK_LAMBDA
	        {
	            [...your code here...]
	        })
	        .onReadValue(CHARACTERISTIC_METHOD_CALLBACK_LAMBDA
	        {
	            [...your code here...]
	        })
	        .onWriteValue(CHARACTERISTIC_METHOD_CALLBACK_LAMBDA
	        {
	            [...your code here...]
	        })
	        .onUpdatedValue(CHARACTERISTIC_UPDATED_VALUE_CALLBACK_LAMBDA
	        {
	            [...your code here...]
	        })
	        .gattDescriptorBegin(name, uuid, flags[])
	            .onEvent(tickFrequency, userData, DESCRIPTOR_EVENT_CALLBACK_LAMBDA
	            {
	                [...your code here...]
	            })
	            .onReadValue(DESCRIPTOR_METHOD_CALLBACK_LAMBDA
	            {
	                [...your code here...]
	            })
	            .onWriteValue(DESCRIPTOR_METHOD_CALLBACK_LAMBDA
	            {
	                [...your code here...]
	            })
	            .onUpdatedValue(DESCRIPTOR_UPDATED_VALUE_CALLBACK_LAMBDA
	            {
	                [...your code here...]
	            })
	        .gattDescriptorEnd()
	    .gattCharacteristicEnd()
	.gattServiceEnd()

# Method reference

The following methods are available within the context of either a characteristic or descriptor.

---
### `onReadValue(callback_or_lambda)`

Register a lambda or callback that is called whenever a Bluetooth client reads the value of a characteristic or descriptor. It is tied to the `ReadValue` method described in the [BlueZ D-Bus GATT API](https://git.kernel.org/pub/scm/bluetooth/bluez.git/plain/doc/gatt-api.txt).

---
### `onWriteValue(callback_or_lambda)`

Register a lambda or callback that is called whenever a Bluetooth client writes to the value of a characteristic or descriptor. It is tied to the `WriteValue` method described in the [BlueZ D-Bus GATT API](https://git.kernel.org/pub/scm/bluetooth/bluez.git/plain/doc/gatt-api.txt).

---
### `onEvent(int tickFrequency, void *pUserData, callback_or_lambda)`

Register a lambda or callback that is called after `tickFrequency` ticks of the periodic timer. Tick events work similar to timer events found in modern GUIs.

Events can be used to update server data, send notifications or perform any other general periodic work. This is a convenience method of GGK and is not part of the Bluetooth standard or BlueZ D-Bus GATT API.

---
### `onUpdatedValue(callback_or_lambda)`

Register a lambda or callback that is called when data is updated internally by the application.

As an application generates or updates its own data, it may notify the server of those updates using the public interface methods `ggkNofifyUpdatedCharacteristic()` and `ggkNofifyUpdatedDescriptor()`. The server will then call the appropriate `onUpdateValue` lambda or callback for the characteristic or descriptor receiving the update. This is a convenience method of GGK and is not part of the Bluetooth standard or BlueZ D-Bus GATT API.

Aside from the application performing data updates, a characteristic or descriptor may modify its own data from within a lambda and trigger this call. For details, see `self.callOnUpdatedValue()` method in the **Lambda reference** section below.

# Lambda reference

Within the context of a lambda there is a `self` parameter that references the parent context (the characteristic or descriptor under which the lambda is registered.)

The following methods are available to through that `self` parameter.

---
#### `T self.getDataValue(const char *name, const T default)`

Gets a named value from the server data (see the section **Server data** for details on how this data is managed.) If `name` is not found, `default` is returned. This is a templated function to allow non-pointer data of any type to be retrieved. For pointer data, see `self.getDataPointer()`.

---
#### `T self.getDataPointer(const char *name, const T default)`

Gets a named pointer from the server data (see the section **Server data** for details on how this data is managed.) If `name` is not found, `default` is returned. This is a templated function to allow pointer data of any type to be retrieved. Note that `T` is a pointer type. For non-pointer values, see `self.getDataValue()`.

---
#### `bool self.setDataValue(const char *name, const T value)`

Sets the named `value` in the server data (see the section **Server data** for details on how this data is managed.) This is a templated function to allow non-pointer data of any type to be set. For pointer data, see `self.setDataPointer()`.

---
#### `bool self.setDataPointer(const char *name, const T pointer)`

Sets the named `pointer` in the server data (see the section **Server data** for details on how this data is managed.) This is a templated function to allow pointer data of any type to be set. Note that `T` is a pointer type. For non-pointer values, see `self.setDataValue()`.

---
#### `bool self.callOnUpdatedValue(self.pConnection, void *pUserData)`

Call the parent context's `onUpdatedValue()` method.

This is a convenience method for situations where a lambda modifies the data of its parent characteristic or descriptor and wishes to notify it's own parent of the change. This simplifies reuse of code by placing all data update management code in the `onUpdatedValue()` registered lambda.

---
#### `void self.methodReturnValue(self.pInvocation, T value, bool wrapInTuple = false)`

Send a value to a Bluetooth client in response to a method call (such as `ReadValue`).

Unlike normal return values from methods within a program, return values in response to Bluetooth method calls must be explicitly sent.

This is a templated function to allow common data types to be automatically placed into a GVariant array of bytes ("ay") with the option of wrapping the response in a tuple ("(ay)"). For a generic `GVariant *` version of this method, see `methodReturnVariant()`.

For information on GVariants, see the [GLib reference manual](https://www.freedesktop.org/software/gstreamer-sdk/data/docs/latest/glib/).

---
#### `void self.methodReturnVariant(self.pInvocation, GVariant *pVariant, bool wrapInTuple = false)`

Send a `GVariant` to a Bluetooth client in response to a method call (such as `ReadValue`).

Unlike normal return values from methods within a program, return values in response to Bluetooth method calls must be explicitly sent.

If `wrapInTuple` is set to true, the `pVariant` is automatically wrapped in a tuple before sending. A convenience function is available for responding with common data types (see `methodReturnValue()`).

For information on GVariants, see the [GLib reference manual](https://www.freedesktop.org/software/gstreamer-sdk/data/docs/latest/glib/).

---
#### `void self.sendChangeNotificationVariant(self.pBusConnection, GVariant *pNewValue)`

Sends a change notification (in BlueZ parlance, a "PropertiesChanged" signal) with an updated `GVariant *` value to subscribers to this characteristic.

This is a generalized method that accepts a `GVariant *`. A templated version is available that supports common types called `sendChangeNotificationValue()`.

For information on GVariants, see the [GLib reference manual](https://www.freedesktop.org/software/gstreamer-sdk/data/docs/latest/glib/).

> NOTE: This method is only available to characteristics.

---
#### `void self.sendChangeNotificationValue(self.pBusConnection, T value)`

Sends a change notification (in BlueZ parlance, a "PropertiesChanged" signal) to subscribers to this characteristic.

This is a helper method that accepts common types. For custom types, there is a form that accepts a `GVariant *`, called `sendChangeNotificationVariant()`.

> NOTE: This method is only available to characteristics.

# Server Data

Server data is maintained by the application. When the application starts the GGK server, it calls `ggkStart()` with two delegates: a data getter and a data setter. These methods are used by the server to retrieve and store server data.

For details on these delegates and their usage, see the comment blocks in `Gobbledegook.h` under the section heading `SERVER DATA`.

# A brief look under the hood

When we build a server description, what we're really doing is building a hierarchical structure of D-Bus objects that conforms to [BlueZ's standards for GATT services](https://git.kernel.org/pub/scm/bluetooth/bluez.git/plain/doc/gatt-api.txt). The `*Begin()` and `*End()` calls are the building blocks for this hierarchy.

GGK uses this server description to build an [XML introspection](https://dbus.freedesktop.org/doc/dbus-specification.html#introspection-format) used to register the object hierarchy with D-Bus. You can view the generated XML for your services by launching the `standalone` application with the `-d` parameter.

GGK also uses the server description to implement the [D-Bus ObjectManager](https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager) interface which is used to describe our services. When we eventually register our application with BlueZ, BlueZ will use our ObjectManager interface to enumerate our services. GGK manages this for us automatically.

# System configuration tips

The following sections may help users with their system configurations.

### BlueZ Version

My distribution didn't have the latest version of BlueZ with proper support for their D-Bus API (it was still considered experimental up until v5.42.) I had to download and build it for myself. I grabbed BlueZ 5.45.

Here's how I built it - this will probably work for most or all Ubuntu users:

	./configure --prefix=/usr --libexecdir=/usr/lib \
	--sysconfdir=/etc --localstatedir=/var --enable-test \
	--enable-manpages --enable-testing --enable-library \
	--enable-maintainer-mode --enable-experimental \
	--enable-deprecated
	make
	service bluetooth stop
	sudo make install           # Helpful for development
	systemctl daemon-reload     # Helpful for development
	service bluetooth start     # Helpful for development

If you're on a Raspberry Pi, you should probably also run `sudo rpi-update` followed by a reboot.

### Configuring BlueZ

The configuration file is located at `/etc/bluetooth/main.conf`. However, you should be fine with defaults here because this server can configure the settings as needed.

### Enable BlueZ debug logs

Edit the file `/lib/systemd/system/bluetooth.service` and set the `ExecStart` line to look like:

	ExecStart=/usr/lib/bluetooth/bluetoothd -d --noplugin=hostname

**`-d`** - it's a good idea to leave this enabled while you are setting up your services in GGK (you can remove it later). This will allow you to `tail -f /var/log/syslog | grep bluetoothd` to get some helpful debug information.

**`--noplugin=hostname`** - this prevents BlueZ from running the 'hostname' plugin, which will rename your device to whatever its hostname is (ignoring your configurations.)

Once you've done this, run the following commands to reload these settings and restart the service:

	sudo systemctl daemon-reload
	sudo service bluetooth stop
	sudo service bluetooth start

### Enabling D-Bus Permissions

In order for our application to communicate over D-Bus, we'll need to ask D-Bus for an *owned name*, which will in effect be our address on D-Bus. D-Bus must be configured to grant us permissions to do this. We'll grant these permissions to user `root`.

You'll need to locate the D-Bus permissions on your box. Likely, you'll find a set of files for this in the directory `/etc/dbus-1/system.d`. Create the file `/etc/dbus-1/system.d/gobbledegook.conf` and give it the contents:

	<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN" "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
	<busconfig>
	  <policy user="root">
	    <allow own="com.gobbledegook"/>
	    <allow send_destination="com.gobbledegook"/>
	    <allow send_destination="org.bluez"/>
	  </policy>
	  <policy at_console="true">
	    <allow own="com.gobbledegook"/>
	    <allow send_destination="com.gobbledegook"/>
	    <allow send_destination="org.bluez"/>
	  </policy>
	  <policy context="default">
	    <deny send_destination="com.gobbledegook"/>
	  </policy>
	</busconfig>

Note the `com.gobbledegook` entries in your new `gobbledegook.conf` file. This must match the service name (the first parameter sent to `ggkStart()` in `standalone.cpp`). If you change the service name from `gobbledegook` to `clownface` in that call to `ggkStart()`, then you'll need to edit the `gobbledegook.conf` file and change all occurrances of `com.gobbledegook` to `com.clownface`.

### Enabling Bluetooth

You don't need to do anything. this server will automatically power on the adapter, enable LE with advertisement.

However, if you want to do this manually, here are a few helpful commands you might try:

	sudo btmgmt -i 0 power off
	sudo btmgmt -i 0 le on
	sudo btmgmt -i 0 connectable on
	sudo btmgmt -i 0 advertising on
	sudo btmgmt -i 0 power on

# Build & Launch

GGK uses the standard autotools build process:

	./configure && make

This will build `libggk.a` then compile `standalone.cpp` into a program that links with `libggk.a`. There is no `make install` as there is nothing to install.

Then run with:

	sudo src/standalone -d

GGK requires super-user privileges when run due to privileges required for D-Bus and HCI sockets. A system can be configured to allow a user to run a GGK server without `sudo`, but that's beyond the scope of this document.

During development, I tend to run these three commands, each in their own terminal:

	sudo tail -f /var/log/syslog | grep bluetoothd
	sudo dbus-monitor --system
	sudo ./src/standalone -d

With no parameters, `standalone` will output only service level output (starting stopping errors, etc.) Additional output parameters are:
	
	`-q`        Quiet - errors only
	`-v`        Verbose - include info log levels
	`-d`        Debug - include debug log levels

# Testing your server

If you don't already have some kind of test harness, you'll probably want something. I've had luck with a free Android app called *nRF Connect*.

# Integration into your own app

Think of Gobbledegook as a template BLE library. You're expected to modify `Server.cpp` and replace the example services with your own. Once you've customized your services, the rest is easy. 

Just link against `libggk.a` and include `include/Gobbledegook.h` to access the public API in your app.  You may notice that the public interface isn't documented here. Instead, it is documented in `include/Gobbledegook.h`.

You can use `standalone.cpp` as a reference on how to get things setup in your code.

# Other handy references

If you decide to dig into the codebase, you'll want to be familiar with D-Bus. If you've never messed with D-Bus, this short [Introduction to D-Bus](https://www.freedesktop.org/wiki/IntroductionToDBus/) will help a lot. GGK uses GLib's GIO for all D-Bus work, which is fully documented in the [GIO Reference Manual](https://developer.gnome.org/gio/stable/). And finally, for the real nitty-gritty on D-Bus be sure to visit the [D-Bus Specification](https://dbus.freedesktop.org/doc/dbus-specification.html).

The [BlueZ D-Bus APIs](https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc) are fully documented. They're brief, but complete. Pay special attention to the [BlueZ D-Bus GATT API description](https://git.kernel.org/pub/scm/bluetooth/bluez.git/plain/doc/gatt-api.txt), which describes how GATT services are implemented over D-Bus.

The [GLib Reference Manual](https://developer.gnome.org/glib/stable/) covers many topics used in GGK, most notably the use of `GVariant` objects for passing data around over D-Bus. Pay special attention to the [GVariant Format Strings](https://developer.gnome.org/glib/stable/gvariant-format-strings.html) page.

# Reference output

The following is the output from a reference tool used to connect to the `standalone` server running with the sample services. The output shows data read from the server over BLE. Note that the hieararchy may not match 1:1 with that of the samples, since some features are automatically provided by BlueZ. In addition, the reference tool periodically writes updates to the `Text string` service (hence the appended text, *"(updated: 3 times)"*.)

	Connected to Gobbledook

	* Service: Device Information
	  > Characteristic: Manufacturer Name String
	    + Value (String): 'Acme Inc.'
	    + Flags: {read}
	  > Characteristic: Model Number String
	    + Value (String): 'Marvin-PA'
	    + Flags: {read}

	* Service: (Example) CPU Info
	  > Characteristic: CPU Count
	    + Value (UInt16): 4
	    + Flags: {read}
	      - Descriptor: Characteristic user description
	        - Value (String): 'This might represent the number of CPUs in the system'
	  > Characteristic: CPU Model
	    + Value (String): 'ARMv7 Processor rev 4 (v7l)'
	    + Flags: {read}
	      - Descriptor: Characteristic user description
	        - Value (String): 'Possibly the model of the CPU in the system'

	* Service: Battery Information
	  > Characteristic: Battery Level
	    + Value (Percent): 75%
	    + Flags: {read, notify}
	      - Descriptor: Client characteristic configuration
	        - Value (Flags): Notifications enabled[false], Indications enabled[false]

	* Service: (Example) asctime()
	  > Characteristic: Ascii Time
	    + Value (String): 'Thu Aug 24 18:43:10 2017'
	    + Flags: {read}
	      - Descriptor: Characteristic user description
	        - Value (String): 'Returns the local time (as reported by POSIX asctime()) each time it is read'

	* Service: (Example) Text string
	  > Characteristic: String value
	    + Value (String): 'Hello, world (updated: 3 times)'
	    + Flags: {read, write, notify}
	      - Descriptor: Client characteristic configuration
	        - Value (Flags): Notifications enabled[false], Indications enabled[false]
	      - Descriptor: Characteristic user description
	        - Value (String): 'A mutable text string used for testing. Read and write to me, it tickles!'

	* Service: Current Time
	  > Characteristic: Current Time
	    + Value (DateTime): Thu, 2017/08/24 18:43:29.00
	    + Flags: {read, notify}
	      - Descriptor: Client characteristic configuration
	        - Value (Flags): Notifications enabled[false], Indications enabled[false]
	  > Characteristic: Local Time Information
	    + Value (LocTime): utcOffset: -5.0 hours, DST (+1h)
	    + Flags: {read}


# Quick-start for the impatient

**Build** with `./configure && make`. This will build `libggk.a` then compile `standalone.cpp` into a program that links with `libggk.a`.

**Run** `sudo ./src/standalone -d`. This will require your machine to be setup correctly with `BlueZ` and working BLE hardware. Don't forget the `sudo`; you'll need it unless you configure the appropriate permissions for your user to access to D-Bus and the HCI socket. Options are: `-e` (errors-only output), `-v` (verbose output), `-d` (debug output)

**Coding** your own BLE services is actually very easy once you become a little familiar with things. Think of Gobbledegook as a template BLE library. You're expected to modify `Server.cpp` and replace the example services with your own. Do yourself a favor and scan through the **Implementing services with GGK** section. It's your quickest path to understanding the foundation.

**Integrating** into your app is easy: just link against `libggk.a` and include `include/Gobbledegook.h` where needed. Use `standalone.cpp` as a template for getting things setup in your code.
