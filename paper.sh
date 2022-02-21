prot=("TcpNewReno" "TcpWestwood" "TcpWestwoodNew")

for i in "${prot[@]}"
do
    echo $i
    ./waf --run "_topology --transport_prot=$i"
done

cd plotting
python3 paper_comparision.py
cd -