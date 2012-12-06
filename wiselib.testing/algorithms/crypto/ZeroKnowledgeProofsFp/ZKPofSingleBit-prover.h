/***************************************************************************
 ** This file is part of the generic algorithm library Wiselib.           **
 ** Copyright (C) 2008,2009 by the Wisebed (www.wisebed.eu) project.      **
 **                                                                       **
 ** The Wiselib is free software: you can redistribute it and/or modify   **
 ** it under the terms of the GNU Lesser General Public License as        **
 ** published by the Free Software Foundation, either version 3 of the    **
 ** License, or (at your option) any later version.                       **
 **                                                                       **
 ** The Wiselib is distributed in the hope that it will be useful,        **
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of        **
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
 ** GNU Lesser General Public License for more details.                   **
 **                                                                       **
 ** You should have received a copy of the GNU Lesser General Public      **
 ** License along with the Wiselib.                                       **
 ** If not, see <http://www.gnu.org/licenses/>.                           **
 ***************************************************************************/

#ifndef __ALGORITHMS_CRYPTO_ZKPOFSINGLEBIT_PROVER_H_
#define __ALGORITHMS_CRYPTO_ZKPOFSINGLEBIT_PROVER_H_

#include "algorithms/crypto/eccfp.h"
#include <string.h>

//Uncomment to enable Debug
#define ENABLE_ZKPOFSINGLEBIT_DEBUG

/* PROTOCOL DESCRIPTION

Zero Knowledge Proof of Single Bit

Prover and Verifier agree on an elliptic curve E over a field Fn , a generator
G in E/Fn and H in E/Fn . Prover knows x and h such that B = x*G + h*H
where h = +1 or -1.He wishes to convince Verifier that he really does know x and
that h really is {+,-}1 without revealing x nor the sign bit.

Protocol Steps

1. Prover generates random s, d, w
2. Prover computes A = s*G - d*(B+hH) and C = w*G
3. If h = -1 Prover swaps(A,C) and sends A,C to Verifier
4. Verifier generates random c and sends c to Prover
5. Prover computes e = c-d and t = w + xe
6. If h =-1 Prover swaps(d,e) and swaps(s,t)
7. Prover sends to Verifier d, e, s, t
8. Verifier checks that e + d = c, s*G = A + d(B + H)
and that t*G = C + e(B-H)
*/

namespace wiselib {

template<typename OsModel_P, typename Radio_P = typename OsModel_P::Radio, typename Debug_P = typename OsModel_P::Debug>
   class ZKPOFSINGLEBITProve
   {
   public:
      typedef OsModel_P OsModel;
      typedef Radio_P Radio;
      typedef Debug_P Debug;

      typedef ZKPOFSINGLEBITProve<OsModel_P, Radio_P, Debug_P> self_t;
	  typedef typename Radio::node_id_t node_id_t;
	  typedef typename Radio::size_t size_t;
	  typedef typename Radio::block_data_t block_data_t;
	  typedef self_t* self_pointer_t;

	  ///@name Construction / Destruction
      ///@{
	  ZKPOFSINGLEBITProve();
      ~ZKPOFSINGLEBITProve();
      ///@}

      ///@name ZKP Control
      ///@{
      ///@}

      //message types
      enum MsgHeaders {
    	  START_MSG = 200,
    	  RAND_MSG = 201,
    	  CONT_MSG = 202,
    	  ACCEPT_MSG = 203,
    	  REJECT_MSG = 204
       };

      int init( Radio& radio, Debug& debug )
	  {
		 radio_ = &radio;
		 debug_ = &debug;
		 return 0;
	  }

	  inline int init();
	  inline int destruct();

	  int enable_radio( void );
	  int disable_radio( void );

      ///@name ZKP functionality
      void key_setup(NN_DIGIT *privkey, Point *pubkey1, Point *pubkey2, int8_t sign);
      void start_proof();
      void send_key();
      void swap_keys(NN_DIGIT *a, NN_DIGIT *b);
      void swap_points(Point *P1, Point *P2);

   protected:
           void receive( node_id_t from, size_t len, block_data_t *data );

   private:

	Radio& radio()
	{ return *radio_; }

	Debug& debug()
	{ return *debug_; }

	typename Radio::self_pointer_t radio_;
	typename Debug::self_pointer_t debug_;

    //necessary class objects
	ECCFP eccfp;
	PMP pmp;

	//rounds of the protocol used for seeding the rand function
	uint8_t rounds;

	//sign bit only the prover knows
	int8_t h;

	//private key that only the prover knows
	NN_DIGIT m[NUMWORDS];

	//private keys to be generated by prover
	NN_DIGIT s[NUMWORDS];
	NN_DIGIT d[NUMWORDS];
	NN_DIGIT w[NUMWORDS];

	//public keys that both prover and verifier know
	//Point B;
	Point P;
	Point C;

	//public keys to send to verifier with the first message
	Point Verify;
	Point Verify2;

	//the random private key that will be received from verifier
	NN_DIGIT c[NUMWORDS];

	//data that will be sent to the verifier
	NN_DIGIT x[NUMWORDS];

   };

	// -----------------------------------------------------------------------
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P>
	ZKPOFSINGLEBITProve<OsModel_P, Radio_P, Debug_P>::
	ZKPOFSINGLEBITProve()
	:radio_(0),
	 debug_(0)
	{}

	// -----------------------------------------------------------------------
	template<typename OsModel_P,
		typename Radio_P,
		typename Debug_P>
	ZKPOFSINGLEBITProve<OsModel_P, Radio_P, Debug_P>::
	~ZKPOFSINGLEBITProve()
	{}

	//-----------------------------------------------------------------------
	template<typename OsModel_P,
				typename Radio_P,
				typename Debug_P>
	int
	ZKPOFSINGLEBITProve<OsModel_P, Radio_P, Debug_P>::
	init( void )
	{
	  enable_radio();
	  return 0;
	}
	//-----------------------------------------------------------------------------
	template<typename OsModel_P,
			typename Radio_P,
			typename Debug_P>
	int
	ZKPOFSINGLEBITProve<OsModel_P, Radio_P, Debug_P>::
	destruct( void )
	{
	  return disable_radio();
	}
	//---------------------------------------------------------------------------
	template<typename OsModel_P,
			typename Radio_P,
			typename Debug_P>
	int
	ZKPOFSINGLEBITProve<OsModel_P, Radio_P, Debug_P>::
	enable_radio( void )
	{
#ifdef ENABLE_ZKPOFSINGLEBIT_DEBUG
		debug().debug( "ZKP Of Single Bit Prove: Boot for %i\n", radio().id() );
#endif

	  radio().enable_radio();
	  radio().template reg_recv_callback<self_t, &self_t::receive>( this );

	  return 0;
	}
	//---------------------------------------------------------------------------
	template<typename OsModel_P,
			typename Radio_P,
			typename Debug_P>
	int
	ZKPOFSINGLEBITProve<OsModel_P, Radio_P, Debug_P>::
	disable_radio( void )
	{
#ifdef ENABLE_ZKPOFSINGLEBIT_DEBUG
		debug().debug( "ZKP of single bit Prove: Disable\n" );
#endif
	  return -1;
	}

	//-----------------------------------------------------------------------
	template<typename OsModel_P,
				typename Radio_P,
				typename Debug_P>
	void
	ZKPOFSINGLEBITProve<OsModel_P, Radio_P, Debug_P>::
	key_setup( NN_DIGIT *privkey, Point *pubkey1, Point *pubkey2, int8_t sign ) {

		rounds=1;

		h=sign;  //get the sign

		//prover's private secret key
		pmp.AssignZero(m, NUMWORDS);
		pmp.Assign(m, privkey, NUMWORDS);

		//get generator P
		eccfp.p_clear(&P);
		eccfp.p_copy(&P, pubkey1);

		//get the final point C = mG + hP
		eccfp.p_clear(&C);
		eccfp.p_copy(&C, pubkey2);
	}
	//--------------------------------------------------------------------------
	template<typename OsModel_P,
				typename Radio_P,
				typename Debug_P>
	void
	ZKPOFSINGLEBITProve<OsModel_P, Radio_P, Debug_P>::
	swap_keys( NN_DIGIT *a, NN_DIGIT *b )
	{
		NN_DIGIT mid[NUMWORDS];
		pmp.AssignZero(mid, NUMWORDS);
		pmp.Assign(mid, a, NUMWORDS);
		pmp.Assign(a, b, NUMWORDS );
		pmp.Assign(b, mid, NUMWORDS);
	}
	//--------------------------------------------------------------------------
	template<typename OsModel_P,
				typename Radio_P,
				typename Debug_P>
	void
	ZKPOFSINGLEBITProve<OsModel_P, Radio_P, Debug_P>::
	swap_points( Point * P1, Point * P2 )
	{
		Point Mid;
		eccfp.p_clear(&Mid);
		eccfp.p_copy(&Mid, P1);
		eccfp.p_copy(P1, P2);
		eccfp.p_copy(P2, &Mid);
	}
	//---------------------------------------------------------------------------
	template<typename OsModel_P,
				typename Radio_P,
				typename Debug_P>
	void
	ZKPOFSINGLEBITProve<OsModel_P, Radio_P, Debug_P>::
	start_proof( void ) {

#ifdef ENABLE_ZKPOFSINGLEBIT_DEBUG
		debug().debug("Debug::Starting ZKP of Single Bit Prover on::%d \n", radio().id() );
#endif

		//if the protocol is booting generate random s,d,w
		rounds++;

		//clear private keys
		pmp.AssignZero(s, NUMWORDS);
		pmp.AssignZero(d, NUMWORDS);
		pmp.AssignZero(w, NUMWORDS);

		//generate private keys
		eccfp.gen_private_key(s, rounds);
		eccfp.gen_private_key(d, rounds+1);
		eccfp.gen_private_key(w, rounds+11);

		//compute Verify = sG - d(B+hH)
		eccfp.p_clear(&Verify);

		Point result;
		Point result1;
		Point result2;
		Point result3;

		eccfp.p_clear(&result);
		eccfp.p_clear(&result1);
		eccfp.p_clear(&result2);
		eccfp.p_clear(&result3);

		//first compute sG
		eccfp.gen_public_key(&result, s);

		//then compute Verify2 = wG
		eccfp.p_clear(&Verify2);
		eccfp.gen_public_key(&Verify2, w);

		if(h == 1)
		{
			//in case where h = +1 no swap is needed

			//compute B+H
			eccfp.c_add_affine(&result1, &C, &P);
			//compute d(B+H)
			eccfp.c_mul(&result2, &result1, d);
			//compute Verify = sG -d(B+H)
			//subtraction in F_{p} if P=(x,y) then -P=(x,-y)
			pmp.ModNeg(result2.y, result2.y, param.p, NUMWORDS);
			eccfp.c_add_affine(&Verify, &result, &result2);
		}
		else
		{
			//in case where h=-1 swap is needed

			//compute B-H
			eccfp.p_copy(&result1, &P);
			pmp.ModNeg(result1.y, result1.y, param.p, NUMWORDS);
			eccfp.c_add_affine(&result2, &C, &result1);
			//compute d(B-H)
			eccfp.c_mul(&result3, &result2, d);
			//compute Verify = sG -d(B-H)
			//subtraction in F_{p} if P=(x,y) then -P=(x,-y)
			pmp.ModNeg(result3.y, result3.y, param.p, NUMWORDS);
			eccfp.c_add_affine(&Verify, &result, &result3);

			swap_points(&Verify, &Verify2);
		}

		//send the message with K,L to verifier
		block_data_t buffer[4*(KEYDIGITS*NN_DIGIT_LEN+1)+1];
		buffer[0]=START_MSG;
		//convert first point to octet and place to buffer
		eccfp.point2octet(buffer+1, 2*(KEYDIGITS*NN_DIGIT_LEN + 1), &Verify, FALSE);
		//convert second point to octet and place to buffer
		eccfp.point2octet(buffer+2*(KEYDIGITS*NN_DIGIT_LEN+1)+1, 2*(KEYDIGITS*NN_DIGIT_LEN + 1), &Verify2, FALSE);

#ifdef ENABLE_ZKPOFSINGLEBIT_DEBUG
		debug().debug("Debug::Finished calculations!Sending 2 verify keys to verifier. ::%d \n", radio().id());
#endif
		radio().send(Radio::BROADCAST_ADDRESS, 4*(KEYDIGITS*NN_DIGIT_LEN+1)+1 , buffer);
	}

	//------------------------------------------------------------------------
	template<typename OsModel_P,
				typename Radio_P,
				typename Debug_P>
	void
	ZKPOFSINGLEBITProve<OsModel_P, Radio_P, Debug_P>::
	send_key( void ) {

#ifdef ENABLE_ZKPOFSINGLEBIT_DEBUG
		debug().debug("Debug::Creating the new private keys ::%d \n", radio().id() );
#endif

		//prover computes e,s,t
		NN_DIGIT e[NUMWORDS];
		NN_DIGIT t[NUMWORDS];

		//clear private keys e,t
		pmp.AssignZero(e, NUMWORDS);
		pmp.AssignZero(t, NUMWORDS);

		//compute e = c - d
		pmp.ModSub(e, c, d, param.r, NUMWORDS);
		//pmp.Mod(e, e, NUMWORDS, param.r, NUMWORDS);

		NN_DIGIT mid2[NUMWORDS];
		pmp.AssignZero(mid2, NUMWORDS);

		//first compute me
		pmp.ModMult(mid2, m, e, param.r, NUMWORDS);
		//then add t = w + me
		pmp.ModAdd(t, w, mid2, param.r, NUMWORDS);

		//in case of -1 swap d <-> e and s <-> t
		if(h == -1)
		{
			swap_keys(d, e);
			swap_keys(s, t);
		}

		//send the message with d,e,s,t to verifier
		block_data_t buffer[4*(KEYDIGITS*NN_DIGIT_LEN+1)+1];
		buffer[0]=CONT_MSG;

		//convert keys to octet and place to buffer
		pmp.Encode(buffer+1, KEYDIGITS * NN_DIGIT_LEN +1, d, NUMWORDS);
		pmp.Encode(buffer+KEYDIGITS * NN_DIGIT_LEN +1+1, KEYDIGITS * NN_DIGIT_LEN +1, e, NUMWORDS);
		pmp.Encode(buffer+2*(KEYDIGITS * NN_DIGIT_LEN +1)+1, KEYDIGITS * NN_DIGIT_LEN +1, s, NUMWORDS);
		pmp.Encode(buffer+3*(KEYDIGITS * NN_DIGIT_LEN +1)+1, KEYDIGITS * NN_DIGIT_LEN +1, t, NUMWORDS);

#ifdef ENABLE_ZKPOFSINGLEBIT_DEBUG
		debug().debug("Debug::Sending the new private keys d,e,s,t to verifier::%d \n", radio().id() );
#endif
		radio().send(Radio::BROADCAST_ADDRESS, 4*(KEYDIGITS*NN_DIGIT_LEN+1)+1, buffer);
	}

	//---------------------------------------------------------------------------
	template<typename OsModel_P,
				typename Radio_P,
				typename Debug_P>
	void
	ZKPOFSINGLEBITProve<OsModel_P, Radio_P, Debug_P>::
	receive( node_id_t from, size_t len, block_data_t *data ) {

		if( from == radio().id() ) return;

		if(data[0]==RAND_MSG)
		{
#ifdef ENABLE_ZKPOFSINGLEBIT_DEBUG
			debug().debug("Debug::Received a random message. Calling the send_key task.::%d \n", radio().id() );
#endif

			//clear the hash and decode the random c received from verifier
			pmp.AssignZero(c, NUMWORDS);
			pmp.Decode(c, NUMWORDS, data+1, KEYDIGITS * NN_DIGIT_LEN +1);

			//calling the send_key task
			send_key();
		}

		if(data[0]==ACCEPT_MSG)
		{
#ifdef ENABLE_ZKPOFSINGLEBIT_DEBUG
			debug().debug("Debug::I got ACCEPTED from the verifier.YESSS!::%d \n", radio().id() );
#endif
		}

		if(data[0]==REJECT_MSG)
		{
#ifdef ENABLE_ZKPOFSINGLEBIT_DEBUG
			debug().debug("Debug::I got REJECTED from the verifier.NOOO!::%d \n", radio().id() );
#endif
		}

	}

} //end of wiselib namespace

#endif /* ZKPOFSINGLEBIT_PROVER_H_ */
