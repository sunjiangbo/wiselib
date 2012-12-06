template<typename Position, typename Radio>
Position get_node_info( Radio* _radio )
{
	if ( _radio->id() == 0x296 )
	{
		return Position( 27, 11, 1 );
	}
	else if ( _radio->id() == 0x295 )
	{
		return Position( 22, 18, 1 );
	}
	else if ( _radio->id() == 0xca3)
	{
		return Position( 28, 18, 1 );
	}
	else if ( _radio->id() == 0x1b77 )
	{
		return Position( 24, 18, 1 );
	}
	else if ( _radio->id() == 0x585 )
	{
		return Position( 15, 18, 1 );
	}
	else if ( _radio->id() == 0x786a)
	{
		return Position( 17, 18, 1 );
	}
	else if ( _radio->id() == 0x1cde )
	{
		return Position( 28, 18, 1 );
	}
	else if ( _radio->id() == 0x0180)
	{
		return Position( 11, 4, 1 );
	}
	else if ( _radio->id() == 0x153d)
	{
		return Position( 11, 4, 1 );
	}
	else if ( _radio->id() == 0x9979)
	{
		return Position( 20, 0, 0 );
	}
	else if ( _radio->id() == 0x0181)
	{
		return Position( 12, 2, 0 );
	}
	return Position( 0, 0, 0 );
}
