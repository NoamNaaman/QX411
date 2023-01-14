/*
 GateKeeper system event types
*/


#define NON_REPORTED_TYPES 128

enum EVENT_TYPES  {
                  EVT_END                    =    0,
                  // event types to be sent to PC
                  EVT_Valid_Entry            =    1,// user ID
                  EVT_Valid_Exit             =    2,// user ID
                  EVT_Invalid_key            =    3, // key code
                  EVT_Invalid_time           =    4,// user ID
                  EVT_Key_timed_out          =    5,// user ID
                  EVT_Invalid_from_to_date   =    6,// user ID
                  EVT_Invalid_door           =    7,// user ID
                  EVT_Key_inactive           =    8,// user ID
                  EVT_Door_forced_open       =    9,
                  EVT_Door_left_open         =   10,
                  EVT_Request_to_Exit        =   11,
                  EVT_Auto_door_open         =   12,
                  EVT_Auto_door_close        =   13,
                  EVT_Communication_Error    =   14,  /////////////////
                  EVT_Controller_fault       =   15,
                  EVT_APB_key_blocked        =   16,// user ID
                  EVT_First_key_valid        =   17,// user ID
                  EVT_Second_key_open        =   18,// user ID
                  EVT_Remote_open            =   19,
                  EVT_Notification           =   20,// user ID   ///////////////////
                  EVT_Unlock_not_permitted   =   21,// user ID
                  EVT_Traffic_limit          =   22,// user ID
                  EVT_power_fault            =   23,
                  EVT_battery_Fault          =   24,
                  EVT_power_Restored         =   25,
                  EVT_wrong_PIN_code         =   26,// user ID
                  EVT_communication_restored =   27,
                  EVT_SD_error               =   28,
                  EVT_fire_condition         =   29,
                                                 
                  EVT_remote_auxiliary       =   30,
                  EVT_battery_Restored       =   31,
                                             
                  EVT_fire_condition_ended   =  130,
                  EVT_tamper_detected        =  131,
                  EVT_tamper_Restored        =  132,
                  EVT_erased_all_keys        =  133,
                  EVT_door_opened            =  134,
                  EVT_door_closed            =  135,
                  EVT_NEW_CTRL               =  136,
                  EVT_remote_entry           =  137,
                  EVT_remote_exit            =  138,
                  EVT_relocked               =  139,
                  EVT_DisDoor                =  140,
                  EVT_EnDoor                 =  141,
                  EVT_RemoteLeaveDoorOpen    =  142,
                  EVT_DoorSetupChanged       =  143,
                  EVT_single_person_inside   =  144,
                  EVT_dead_man               =  145,
                  EVT_burglary               =  146,
                  EVT_LAST 
                  };

