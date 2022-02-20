/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Siddharth Gangadhar <siddharth@ittc.ku.edu>,
 *          Truc Anh N. Nguyen <annguyen@ittc.ku.edu>,
 *          Greeshma Umapathi
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

#include "tcp-westwood-new.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "rtt-estimator.h"
#include "tcp-socket-base.h"

NS_LOG_COMPONENT_DEFINE("TcpWestwoodNew");

namespace ns3
{

    // need to understand this ?
    NS_OBJECT_ENSURE_REGISTERED(TcpWestwoodNew);

    TypeId
    TcpWestwoodNew::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::TcpWestwoodNew")
                                .SetParent<TcpNewReno>()
                                .SetGroupName("Internet")
                                .AddConstructor<TcpWestwoodNew>()
                                .AddAttribute("FilterType", "Use this to choose no filter or Tustin's approximation filter",
                                              EnumValue(TcpWestwoodNew::TUSTIN), MakeEnumAccessor(&TcpWestwoodNew::m_fType),
                                              MakeEnumChecker(TcpWestwoodNew::NONE, "None", TcpWestwoodNew::TUSTIN, "Tustin"))
                                .AddTraceSource("EstimatedBW", "The estimated bandwidth",
                                                MakeTraceSourceAccessor(&TcpWestwoodNew::m_currentBW),
                                                "ns3::TracedValueCallback::Double");
        return tid;
    }

    TcpWestwoodNew::TcpWestwoodNew(void) : TcpNewReno(),
                                           m_currentBW(0),
                                           m_lastSampleBW(0),
                                           m_lastBW(0),
                                           m_ackedSegments(0),
                                           m_IsCount(false),
                                           m_lastAck(0),
                                           ////////////////////////////////////// vodro \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\/

                                           _bw_ratio(1)
    ////////////////////////////////////// vodro \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\/

    {
        NS_LOG_FUNCTION(this);
    }

    TcpWestwoodNew::TcpWestwoodNew(const TcpWestwoodNew &sock) : TcpNewReno(sock),
                                                                 m_currentBW(sock.m_currentBW),
                                                                 m_lastSampleBW(sock.m_lastSampleBW),
                                                                 m_lastBW(sock.m_lastBW),
                                                                 //  m_pType(sock.m_pType),
                                                                 m_fType(sock.m_fType),
                                                                 m_IsCount(sock.m_IsCount)
    {
        // NS_LOG_DEBUG("TcpWestwoodNew::GetTypeId called !");

        NS_LOG_FUNCTION(this);
        NS_LOG_LOGIC("Invoked the copy constructor");
    }

    TcpWestwoodNew::~TcpWestwoodNew(void)
    {
    }

    void
    TcpWestwoodNew::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t packetsAcked,
                              const Time &rtt)
    {
        NS_LOG_FUNCTION(this << tcb << packetsAcked << rtt);

        if (rtt.IsZero())
        {
            NS_LOG_WARN("RTT measured is zero!");
            return;
        }

        m_ackedSegments += packetsAcked;
        ////////////////////////////////////// vodro \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\/

        // NS_LOG_DEBUG("TcpWestwoodNew::PktsAcked " << m_ackedSegments);

        ////////////////////////////////////// vodro \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\/

        // need some work here
        //        if (m_pType == TcpWestwood::WESTWOOD)
        //        {
        EstimateBW(rtt, tcb);
        //        }
        //        else if (m_pType == TcpWestwood::WESTWOODPLUS)
        //        {
        //            if (!(rtt.IsZero () || m_IsCount))
        //            {
        //                m_IsCount = true;
        //                m_bwEstimateEvent.Cancel ();
        //                m_bwEstimateEvent = Simulator::Schedule (rtt, &TcpWestwood::EstimateBW,
        //                                                         this, rtt, tcb);
        //            }
        //        }
    }

    void
    TcpWestwoodNew::EstimateBW(const Time &rtt, Ptr<TcpSocketState> tcb)
    {

        NS_LOG_FUNCTION(this);

        NS_ASSERT(!rtt.IsZero());

        m_currentBW = m_ackedSegments * tcb->m_segmentSize / rtt.GetSeconds();

        //        if (m_pType == TcpWestwood::WESTWOOD)
        //        {
        // NS_LOG_DEBUG("TcpWestWoodNew::EstimateBW before ==" << m_currentBW);

        Time currentAck = Simulator::Now();
        m_currentBW = m_ackedSegments * tcb->m_segmentSize / (currentAck - m_lastAck).GetSeconds();

        // NS_LOG_DEBUG("TcpWestWoodNew::EstimateBW after ==" << m_currentBW << " rtt : " << rtt.GetSeconds() << " time diff : " << (currentAck - m_lastAck).GetSeconds());

        m_lastAck = currentAck;
        //        }
        //        else if (m_pType == TcpWestwood::WESTWOODPLUS)
        //        {
        //            m_currentBW = m_ackedSegments * tcb->m_segmentSize / rtt.GetSeconds ();
        //            m_IsCount = false;
        //        }

        m_ackedSegments = 0;
        NS_LOG_LOGIC("Estimated BW: " << m_currentBW);

        // Filter the BW sample

        double alpha = 0.9;

        if (m_fType == TcpWestwoodNew::NONE)
        {
        }
        else if (m_fType == TcpWestwoodNew::TUSTIN)
        {
            double sample_bwe = m_currentBW;
            m_currentBW = (alpha * m_lastBW) + ((1 - alpha) * ((sample_bwe + m_lastSampleBW) / 2));
            ////////////////////////////////////// vodro \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\/
            // NS_LOG_DEBUG("TcpWestwoodNew::EstimateBW == "
            //              << "_bw_ratio before: " << _bw_ratio);
            // double _prev_ratio = _bw_ratio;
            // NS_LOG_DEBUG("TcpWestwoodNew::EstimateBW == " << m_lastBW << " -> " << m_currentBW);
            _bw_ratio = m_currentBW / m_lastBW;
            // NS_LOG_DEBUG("TcpWestwoodNew::EstimateBW == "
            //              << _prev_ratio << " -> " << _bw_ratio << " \n");

            ////////////////////////////////////// vodro \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\/

            m_lastSampleBW = sample_bwe;
            m_lastBW = m_currentBW;
        }

        NS_LOG_LOGIC("Estimated BW after filtering: " << m_currentBW);
    }

    uint32_t
    TcpWestwoodNew::GetSsThresh(Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight)
    {
        // std::cout << "TcpWestwoodNew:: GetSsThresh" << std::endl;

        NS_UNUSED(bytesInFlight);
        NS_LOG_LOGIC("CurrentBW: " << m_currentBW << " minRtt: " << tcb->m_minRtt << " ssthresh: " << m_currentBW * static_cast<double>(tcb->m_minRtt.GetSeconds()));

        return std::max(2 * tcb->m_segmentSize,
                        uint32_t(m_currentBW * static_cast<double>(tcb->m_minRtt.GetSeconds())));
    }

    /**
     * \brief TcpWestwoodNew congestion avoidance
     *
     * During congestion avoidance, cwnd is incremented by our paper
     * segment per round-trip time (RTT).
     *
     * \param tcb internal congestion state
     * \param segmentsAcked count of segments acked
     */
    void
    TcpWestwoodNew::CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
    {
        NS_LOG_FUNCTION(this << tcb << segmentsAcked);
        // NS_LOG_DEBUG("TcpWestwoodNew::CongestionAvoidance");
        if (segmentsAcked > 0)
        {
            double adder = static_cast<double>(tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get();

            adder = std::max(1.0, adder);

            ////////////////////////////////////// vodro \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\/

            if (_bw_ratio >= 1.25)
            {
                adder *= 2;
                NS_LOG_DEBUG("TcpWestwoodNew::CongestionAvoidance := "
                             << "bandwidth ratio greater than 1.25 ");
            }
            else if (_bw_ratio >= 1)
            {
                adder *= 1;
                NS_LOG_DEBUG("TcpWestwoodNew::CongestionAvoidance := "
                             << " 1 <= bandwidth ratio < 1.5  ");
            }
            else
            {
                adder = 0;
                NS_LOG_DEBUG("TcpWestwoodNew::CongestionAvoidance := "
                             << " 0 < bandwidth ratio  ");
            }
            NS_LOG_DEBUG(" < " << Simulator::Now().GetSeconds() << " > " "TcpWestwoodNew::CongestionAvoidance := adding : " << adder << " cWnd : " << tcb->m_cWnd.Get() << " b_ratio : " << _bw_ratio);

            ////////////////////////////////////// vodro \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\/

            tcb->m_cWnd += static_cast<uint32_t>(adder);
            NS_LOG_INFO("In CongAvoid, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
        }
    }

    Ptr<TcpCongestionOps>
    TcpWestwoodNew::Fork()
    {
        return CreateObject<TcpWestwoodNew>(*this);
    }

} // namespace ns3
