for s in `seq 1 15`; do 
    ssh linux$s "bash ~/Documents/CN/CN-Project1/test/deploy.sh"
done
