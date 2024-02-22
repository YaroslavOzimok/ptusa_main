#include "statistic_manager.h"

auto_smart_ptr < statistic_manager > statistic_manager::instance;
//-----------------------------------------------------------------------------
device_with_statistic::device_with_statistic( device * dev,
	int device_resource ) : par( saved_params_float( 3 ) ), working_time( 0 ),
	device_resource( device_resource ), dev( dev )
	{
	prev_device_state = dev->get_state();
	cur_stat = par[ 1 ];
	}
//-----------------------------------------------------------------------------
int device_with_statistic::get_cur_device_stat()
	{
	return cur_stat;
	}
//-----------------------------------------------------------------------------
int device_with_statistic::get_device_working_time()
	{
	return working_time;
	}
//-----------------------------------------------------------------------------
float device_with_statistic::get_cur_device_wear()
	{
	return ( cur_stat / (float)device_resource ) * 100;
	}
//-----------------------------------------------------------------------------
void device_with_statistic::check_state_changes()
	{
	if( prev_device_state != dev->get_state() && prev_device_state == 0 )
		{
		prev_device_state = dev->get_state();
		cur_stat++;
		par.save( 1, cur_stat );
		}
	else
		{
		prev_device_state = dev->get_state();
		}
	}
//-----------------------------------------------------------------------------
int device_with_statistic::save_common_stat( char *buff )
	{
	return fmt::format_to_n( buff, MAX_COPY_SIZE,
		"{}={{STAT_CH={:d}, STAT_RS={:d}, STAT_WR={:.2f}, STAT_WT={:d}}},",
		dev->get_name(), cur_stat, device_resource, get_cur_device_wear(),
		working_time ).size;
	}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
statistic_manager::statistic_manager()
	{
	G_DEVICE_CMMCTR->add_device( this );
	}
//-----------------------------------------------------------------------------
statistic_manager::~statistic_manager()
	{
	for( auto dev : devs_with_stat )
		{
		delete dev;
		dev = nullptr;
		}
	}
//-----------------------------------------------------------------------------
void statistic_manager::add_new_dev_with_stat( device *dev,
	int device_resource )
	{
	auto new_dev = new device_with_statistic( dev, device_resource );
	devs_with_stat.push_back( new_dev );
	}
//-----------------------------------------------------------------------------
void statistic_manager::evaluate()
	{
	for( auto dev : devs_with_stat )
		{
		dev->check_state_changes();
		}
	}
//-----------------------------------------------------------------------------
int statistic_manager::save_device( char *buff )
	{
	int res = sprintf( buff, "t.%s = t.%s or {}\nt.%s=\n\t{\n",
		get_name_in_Lua(), get_name_in_Lua(), get_name_in_Lua() );

	for( auto dev : devs_with_stat )
		{
		res += dev->save_common_stat( buff + res );
		}

	res += sprintf( buff + res, "\t}\n" );
	return res;
	}
//-----------------------------------------------------------------------------
const char *statistic_manager::get_name_in_Lua() const
	{
	return "Statistic_manager";
	}
//-----------------------------------------------------------------------------
statistic_manager* statistic_manager::get_instance()
	{
	if( instance.is_null() )
		{
		instance = new statistic_manager();
		}

	return instance;
	}
//-----------------------------------------------------------------------------
statistic_manager* G_STATISTIC_MANAGER()
	{
	return statistic_manager::get_instance();
	}