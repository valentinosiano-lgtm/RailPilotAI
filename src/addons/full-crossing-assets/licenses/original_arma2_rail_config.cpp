#define true 1
#define false 0
#define private 0
#define protected 1
#define public 2

class CfgPatches
{
	class CAStructures_Rail
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.10;
		requiredAddons[] = {CAStructures};
	};
};

class CfgVehicles
{
	class House;
	class Land_Rail_Semafor: House
	{
		scope=protected;
		model = "\CA\Structures\Rail\Rail_Misc\rail_Semafor";
		destrType=DestructTree;
		class MarkerLights
		{
			class GreenStill
			{
				name = "zeleny pozicni";
				color[]={0.0042,0.032,0.003,1.0};
				ambient[] = {0.01,0.1,0.01,1.0};
				brightness = 0.01;
				blinking = false;
			};
		};
	};
	class Land_Rail_Zavora: House
	{
		scope=protected;
		model = "\CA\Structures\Rail\Rail_Misc\rail_Zavora";
		destrType=DestructTree;
		class MarkerLights
		{
			class WhiteBlinking
			{
				name = "bily pozicni blik";
				color[]={0.01,0.01,0.03,1.0};
				ambient[]={0.01,0.01,0.0056,1.0};
				brightness = 0.01;
				blinking = true;
			};
		};
	};
	class Land_wagon_box: House
	{
		scope = public;
		displayName = $STR_WAGON_BOX;
		model = "\Ca\Structures\Rail\Rail_Wagon\wagon_box";
//		armor = 500;
		ladders[]={{"start1","end1"}};
		vehicleClass="Misc";
	};
	class Land_wagon_flat: House
	{
		scope = public;
		displayName = $STR_WAGON_FLAT;
		model = "\Ca\Structures\Rail\Rail_Wagon\wagon_flat";
//		armor = 500;
		ladders[]={{"start1","end1"},{"start2","end2"},{"start3","end3"},{"start4","end4"}};
		vehicleClass="Misc";
	};
	class Land_wagon_tanker: House
	{
		scope = public;
		displayName = $STR_WAGON_TANKER;
		model = "\Ca\Structures\Rail\Rail_Wagon\wagon_tanker";
//		armor = 500;
		ladders[]={{"start1","end1"},{"start2","end2"}};
		vehicleClass="Misc";
	};
	class Land_loco_742_blue: House
	{
		scope = public;
		displayName = $STR_LOCO_BLUE;
		model = "\Ca\Structures\Rail\Rail_Loco\loco_742_blue";
		vehicleClass="Misc";
	};
	class Land_loco_742_red: Land_loco_742_blue
	{
		displayName = $STR_LOCO_RED;
		model = "\Ca\Structures\Rail\Rail_Loco\loco_742_red";
	};
};
