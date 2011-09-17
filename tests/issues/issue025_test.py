# Issue 25 - Netcdf module does not filter names that start with a digit
# http://code.google.com/p/pygeode/issues/detail?id=25

from pygeode.formats import netcdf as nc
from pygeode.axis import Lat
from pygeode.var import Var

def test_issueXXX():
  lat = Lat([80,70,60])
  var = Var(axes=[lat], values=[1,2,3], name='2B')

  # Save the variable
  nc.save ("issueXXX_test.nc", var)

  # This may crash in some versions of the netcdf library.

  # Even if it doesn't crash, it's a good idea to enforce the legal
  # netcdf names

  f = nc.open("issueXXX_test.nc")

  assert len(f.vars) == 1
  # Must not start with a digit (should have been filtered)
  assert not f.vars[0].name[0].isdigit()

