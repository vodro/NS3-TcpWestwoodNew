prot=("TcpNewReno" "TcpWestwood" "TcpWestwoodNew")

# prot=("TcpWestwoodNew")

for i in "${prot[@]}"
do
    echo $i
    ./waf --run "paper_third --transport_prot=$i"
done

cd plotting
python3 paper_comparision.py
cd -