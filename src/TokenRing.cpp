/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2015 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the processing element
 */

#include "TokenRing.h"

void TokenRing::updateTokenPacket(int channel)
{
	if (flag[channel][token_position[channel]]->read() == RELEASE_CHANNEL)
	{
	    // number of hubs of the ring
	    int num_hubs = rings_mapping[channel].size();

	    token_position[channel] = (token_position[channel]+1)%num_hubs;
	    LOG << "Token of channel " << channel << " has been assigned to hub " <<  rings_mapping[channel][token_position[channel]].first << endl;

	    current_token_holder[channel]->write(rings_mapping[channel][token_position[channel]].first);
	}
}

void TokenRing::updateTokenMaxHold(int channel)
{
    // TODO TURI: controllare struttura dati per hold count, troppo
    // complessa, per esempio il contatore dipende solo dal canale,
    // non pure dalla posizione del token
	if (--rings_mapping[channel][token_position[channel]].second == 0 ||
		flag[channel][token_position[channel]]->read() == RELEASE_CHANNEL)
	{
	    rings_mapping[channel][token_position[channel]].second =
	    GlobalParams::hub_configuration[rings_mapping[channel][token_position[channel]].first].txChannels[channel].maxHoldCycles;
	    // number of hubs of the ring
	    int num_hubs = rings_mapping[channel].size();

	    token_position[channel] = (token_position[channel]+1)%num_hubs;
	    LOG << "Token of channel " << channel << " has been assigned to hub " <<  rings_mapping[channel][token_position[channel]].first << endl;

	    current_token_holder[channel]->write(rings_mapping[channel][token_position[channel]].first);
	}

	current_token_expiration[channel]->write(rings_mapping[channel][token_position[channel]].second);
}

void TokenRing::updateTokenHold(int channel)
{
	if (--rings_mapping[channel][token_position[channel]].second == 0)
	{
	    rings_mapping[channel][token_position[channel]].second =
	    GlobalParams::hub_configuration[rings_mapping[channel][token_position[channel]].first].txChannels[channel].maxHoldCycles;
	    // number of hubs of the ring
	    int num_hubs = rings_mapping[channel].size();

	    token_position[channel] = (token_position[channel]+1)%num_hubs;
	    LOG << "Token of channel " << channel << " has been assigned to hub " <<  rings_mapping[channel][token_position[channel]].first << endl;

	    current_token_holder[channel]->write(rings_mapping[channel][token_position[channel]].first);
	}

	current_token_expiration[channel]->write(rings_mapping[channel][token_position[channel]].second);
}

void TokenRing::updateTokens()
{
    if (reset.read()) {
        for (map<int,ChannelConfig>::iterator i = GlobalParams::channel_configuration.begin();
                i!=GlobalParams::channel_configuration.end(); 
                i++)
	current_token_holder[i->first]->write(0);

    } 
    else 
    {

        for (map<int,ChannelConfig>::iterator i = GlobalParams::channel_configuration.begin(); i!=GlobalParams::channel_configuration.end(); i++)
        {
	    int channel = i->first;

	    switch (getPolicy(channel))
	    {
		case TOKEN_PACKET:
		    updateTokenPacket(channel);
		    break;
		case TOKEN_HOLD:
		    updateTokenHold(channel);
		    break;
		case TOKEN_MAX_HOLD:
		    updateTokenMaxHold(channel);
		    break;

		default: assert(false);
	    }
        }
    }
}


void TokenRing::attachHub(int channel, int hub, sc_in<int>* hub_token_holder_port, sc_in<int>* hub_token_expiration_port, sc_out<int>* hub_flag_port)
{
    // If port for requested channel is not present, create the
    // port and connect a signal
    if (!current_token_holder[channel])
    {
    	current_token_holder[channel] = new sc_out<int>();
    	current_token_expiration[channel] = new sc_out<int>();

    	token_holder_signals[channel] = new sc_signal<int>();
    	token_expiration_signals[channel] = new sc_signal<int>();

	current_token_holder[channel]->bind(*(token_holder_signals[channel]));
	current_token_expiration[channel]->bind(*(token_expiration_signals[channel]));

	// checking max hold cycles vs wireless transmission latency
	// consistency
        double delay_ps = 1000*GlobalParams::flit_size/GlobalParams::channel_configuration[channel].dataRate;
	int cycles = ceil(delay_ps/CLOCK_PERIOD);
	int max_hold_cycles = GlobalParams::hub_configuration[hub].txChannels[channel].maxHoldCycles;
	assert(cycles< max_hold_cycles);

    }	

    flag[channel][hub] = new sc_in<int>();
    flag_signals[channel][hub] = new sc_signal<int>();
    flag[channel][hub]->bind(*(flag_signals[channel][hub]));
    hub_flag_port->bind(*(flag_signals[channel][hub]));

    // Connect tokenring to hub
    hub_token_holder_port->bind(*(token_holder_signals[channel]));
    hub_token_expiration_port->bind(*(token_expiration_signals[channel]));

    LOG << "Attaching Hub " << hub << " to the token ring for channel " << channel << endl;
    rings_mapping[channel].push_back(pair<int, int>(hub, GlobalParams::hub_configuration[hub].txChannels[channel].maxHoldCycles));
}


