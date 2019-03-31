# About

Gazebo Ros plugin to steer an actor with twist messages via ros.
Based on actor_plugin from https://bitbucket.org/osrf/gazebo/raw/default/plugins/ActorPlugin.cc

# Usage

Example actor tag in a .world file:
```
<actor name="actor1">
      <pose>0 1 1.25 0 0 0</pose>
      <skin>
        <filename>moonwalk.dae</filename>
        <scale>1.0</scale>
      </skin>
      <animation name="walking">
        <filename>walk.dae</filename>
        <scale>1.000000</scale>
        <interpolate_x>true</interpolate_x>
      </animation>
      <plugin name="actor1_plugin" filename="libros_actor_cmd_vel_plugin.so">
        <animation_factor>5.1</animation_factor>
      </plugin>
    </actor>
```

The actor will listen for twist messages on the ros topic `/actor1/cmd_vel`.

Hint: Use with https://github.com/CentralLabFacilities/keyboard_teleop to steer via teleop (this fork supports specifying a published topic).