echo "cleaning..."
echo rm -rf ../web_pages/*
mkdir ../web_pages/h
echo "gzipping files..."
gzip -9 -f -c ./h/index.html > ../web_pages/index.html
//cp ./h/index.html ../web_pages/h/index.html
echo "making tfs files..."
./mktfs ../web_pages
echo "moving tfs_data.c to .."
mv tfs_data.c ../src
echo "Done."
